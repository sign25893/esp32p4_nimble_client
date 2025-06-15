#include <string>
#include <NimBLEDevice.h>

// Глобальные переменные
const NimBLEAdvertisedDevice* advDevice = nullptr;
static NimBLERemoteCharacteristic* pCharToWrite = nullptr;
static std::string targetMacAddress = "";
static int receivedIntValue = 0;  // Последнее принятое целое число
static float receivedFloatValue = 0.0f;  // Последнее принятое значение с плавающей точкой
static int sendValue = 0;  // Значение для отправки

/** Класс для обработки событий клиента */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        printf("\nПодключено к устройству!\n");
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        printf("\nОтключено. Причина %d. Продолжаем поиск...\n", reason);
        advDevice = nullptr;
    }
} clientCallbacks;

/** Класс для обработки событий сканирования */
class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {  // используем const
        printf("\nНайдено устройство: %s\n", advertisedDevice->toString().c_str());

        if (advertisedDevice->getAddress().toString() == targetMacAddress) {
            printf("Целевое устройство найдено: %s\n", targetMacAddress.c_str());
            advDevice = advertisedDevice;
        }
    }
} scanCallbacks;


/** Обработчик уведомлений */
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    printf("\n[Уведомление] от %s\n", pRemoteCharacteristic->getClient()->getPeerAddress().toString().c_str());
    printf("   Сервис: %s\n", pRemoteCharacteristic->getRemoteService()->getUUID().toString().c_str());
    printf("   Характеристика: %s\n", pRemoteCharacteristic->getUUID().toString().c_str());

    printf("   Полученные данные (HEX): ");
    for (size_t i = 0; i < length; ++i) {
        printf("%02X ", pData[i]);
    }
    printf("\n");

    // Интерпретация как float
    if (length >= 4) {
        union {
            uint8_t bytes[4];
            float fValue;
        } converter;

        memcpy(converter.bytes, pData, 4);
        receivedFloatValue = converter.fValue;
        printf("   Измеренное значение (Float): %.2f мм\n", receivedFloatValue);
    }

    printf("——————————————————————————————————————\n");
}


/** Отправка данных */
void sendIntegerValue() {
    if (pCharToWrite && pCharToWrite->canWrite()) {
        uint8_t data[2] = { (uint8_t)(sendValue >> 8), (uint8_t)(sendValue & 0xFF) };
        pCharToWrite->writeValue(data, sizeof(data), false);
        printf("📤 Отправлено: %d\n", sendValue);
    }
}

/** Функция подключения к серверу */
bool connectToServer() {
    if (!advDevice) {
        printf("Устройство не найдено. Повторяем поиск...\n");
        return false;
    }

    NimBLEClient* pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(&clientCallbacks, false);

    if (!pClient->connect(advDevice)) {
        printf("Ошибка подключения!\n");
        return false;
    }

    printf("Подключено к %s\n", pClient->getPeerAddress().toString().c_str());

    NimBLERemoteService* pSvc = pClient->getService("00000000-0001-0001-0000-CECCC8CAD12E");
    if (!pSvc) {
        printf("Сервис не найден!\n");
        pClient->disconnect();
        return false;
    }

    NimBLERemoteCharacteristic* pChr = pSvc->getCharacteristic("00000000-0001-0001-0000-CECCC8CAD12E");
    if (!pChr) {
        printf("Характеристика не найдена!\n");
        pClient->disconnect();
        return false;
    }

    if (pChr->canNotify()) {
        pChr->subscribe(true, notifyCB);
    }

    printf("Подписка на уведомления успешно оформлена!\n");

    pCharToWrite = pChr; // Запоминаем характеристику для записи

    while (pClient->isConnected()) {
        sendIntegerValue(); // Отправляем текущее значение каждую секунду
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Отключение... Возвращаемся к поиску.\n");
    return false;
}

/** Инициализация BLE */
void ble_init() {
    printf("\nЗапуск BLE-клиента\n");
    NimBLEDevice::init("NimBLE-Client");
    NimBLEDevice::setPower(3);

    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks, false);
    pScan->setInterval(100);
    pScan->setWindow(100);
    pScan->setActiveScan(true);
}

/** Подключение к устройству с указанным MAC-адресом */
void ble_connect(const std::string& macAddress) {
    targetMacAddress = macAddress;
    NimBLEScan* pScan = NimBLEDevice::getScan();

    while (true) {
        if (!advDevice) {
            printf("\nПоиск устройства...\n");
            pScan->start(5000, false);
        }

        if (advDevice) {
            if (connectToServer()) {
                printf("\nПодключено! Ожидаем уведомлений...\n");
            } else {
                printf("\nНе удалось подключиться. Повторяем поиск...\n");
                advDevice = nullptr;
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

/** Геттеры */
int ble_getReceivedIntValue() {
    return receivedIntValue;
}

float ble_getReceivedFloatValue() {
    return receivedFloatValue;
}

/** Сеттер */
void ble_setSendValue(int value) {
    sendValue = value;
}
