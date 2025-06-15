#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"

#include <ble.cpp>  

void ble_task(void *pvParameters) {

    printf("\nЗапуск BLE-клиента\n");
    NimBLEDevice::init("NimBLE-Client");
    NimBLEDevice::setPower(3);

    NimBLEScan* pScan = NimBLEDevice::getScan();
        pScan->setScanCallbacks(&scanCallbacks, false);
        pScan->setInterval(100);
        pScan->setWindow(100);
        pScan->setActiveScan(true);

    while (true) {

        targetMacAddress = "00:11:22:33:44:55";
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

        ESP_LOGI("BLE", "1SEC++");

        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }    
}

extern "C" void app_main() {

    xTaskCreate(ble_task, "ble_task", 2*1024, NULL, 5, NULL);

}
