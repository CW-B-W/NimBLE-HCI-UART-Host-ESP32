#include "esp_log.h"

#include "transport/uart/ble_hci_uart.h"

#include "NimBLEDevice.h"

extern "C" void app_main()
{
    ble_hci_uart_init();

    NimBLEDevice::init("NimBLE-ESP32");
    NimBLEServer *pServer = NimBLEDevice::createServer();
    NimBLEService* pDeadService = pServer->createService("DEAD");
    NimBLECharacteristic* pBeefCharacteristic = pDeadService->createCharacteristic(
            "BEEF",
            NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE
    );
    pBeefCharacteristic->setValue("Burger");
    pDeadService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pDeadService->getUUID());
    pAdvertising->start();

    ESP_LOGI("app_main", "Advertising Started");
}
