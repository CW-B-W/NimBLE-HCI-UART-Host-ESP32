#include "esp_log.h"

#include "transport/uart/ble_hci_uart.h"

#include "NimBLEDevice.h"
#include "NimBLECharacteristic.h"

class BEEFCallbacks: public BLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic* pCharacteristic)
    {
       /*
        *   Must use a variable to store pCharacteristic->getValue()
        *   Otherwise it may be overwrite by other local variables
        */
        NimBLEAttValue attval = pCharacteristic->getValue();
        const char *val = attval.c_str();
        ESP_LOGI("BEEFCallback::onRead()", "val = %s", val);

       /* 
        *   This is the wrong example, the value printed is incorrect
        *   I speculate it's because `pCharacteristic->getValue()`
        *   is not stored in a variable, after
        *   `pCharacteristic->getValue().c_str()` is executed
        *   it's automatically freed.
        *   Therefore when `ESP_LOGI(...)` is executed, it overwrites
        *   the content in ``pCharacteristic->getValue().c_str()`
        */
        // const char *val = pCharacteristic->getValue().c_str();
        // ESP_LOGI("BEEFCallback::onRead()", "val = %s", val);
    }

    void onWrite(NimBLECharacteristic* pCharacteristic)
    {
        NimBLEAttValue attval = pCharacteristic->getValue();
        const char *val = attval.c_str();
        ESP_LOGI("BEEFCallback::onWrite()", "val = %s", val);
    }
};

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
    pBeefCharacteristic->setCallbacks(new BEEFCallbacks());
    pBeefCharacteristic->setValue("Burger");
    pDeadService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pDeadService->getUUID());
    pAdvertising->start();

    ESP_LOGI("app_main", "Advertising Started");
}
