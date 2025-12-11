#include "JCON_BLE.h"

void ServerCallbacks::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
    if (_pInstance) {
        _pInstance->deviceConnected = true;
        pServer->updateConnParams(param->connect.remote_bda, 0x06, 0x0C, 0, 100);
        
        Serial.println("[EVENT] Device connected. Requested low latency.");
    }
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    if (_pInstance) {
        _pInstance->deviceConnected = false;
        Serial.println("[EVENT] Device disconnected. Restarting advertising...");
        BLEDevice::startAdvertising();
    }
}

void WriteCharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue(); 

    if (rxValue.length() > 0) {
        
        // デバッグ用: 受信データ表示
        // Serial.print("RX: ");
        // Serial.println(rxValue);

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        
        DeserializationError error = deserializeJson(doc, rxValue);

        if (error) {
            Serial.print(F("[ERROR] JSON parse failed: "));
            Serial.println(error.f_str());
            return;
        }

        if (_parseCallback) {
            _parseCallback(doc, _dataPtr);
        }
    }
}

JConBLE::JConBLE() {
}

void JConBLE::begin(const char* deviceName, void* dataPtr, ParseCallback parseCallback) {
    
    Serial.begin(115200); 
    Serial.println("Starting JConBLE server...");
    
    BLEDevice::init(deviceName); 
    pServer = BLEDevice::createServer();
    pServerCallbacks = new ServerCallbacks(this); 
    pServer->setCallbacks(pServerCallbacks); 

    pService = pServer->createService(SERVICE_UUID);
    
    // RX (書き込み用)
    pWriteCharacteristic = pService->createCharacteristic(
                                        WRITE_CHAR_UUID,
                                        BLECharacteristic::PROPERTY_WRITE | 
                                        BLECharacteristic::PROPERTY_WRITE_NR
                                    );
    pWriteCallbacks = new WriteCharacteristicCallbacks(dataPtr, parseCallback);
    pWriteCharacteristic->setCallbacks(pWriteCallbacks);
    pService->addCharacteristic(pWriteCharacteristic); 

    // TX (通知用)
    pNotifyCharacteristic = pService->createCharacteristic(
                                        NOTIFY_CHAR_UUID,
                                        BLECharacteristic::PROPERTY_NOTIFY
                                    );
    pNotifyCharacteristic->addDescriptor(new BLE2902());
    pService->addCharacteristic(pNotifyCharacteristic);

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); 
    pAdvertising->setMaxPreferred(0x12);
    
    BLEDevice::startAdvertising();
    Serial.println("BLE Advertising started.");
}

void JConBLE::send(String jsonString) {
    if (deviceConnected && pNotifyCharacteristic != NULL) {
        pNotifyCharacteristic->setValue(jsonString.c_str());
        pNotifyCharacteristic->notify();
    }
}