#include "JCON_BLE.h"

void ServerCallbacks::onConnect(BLEServer* pServer) {
    if (_pInstance) {
        _pInstance->deviceConnected = true;
        Serial.println("[EVENT] Device connected.");
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
    String tempValue = pCharacteristic->getValue(); 
    std::string rxValue = tempValue.c_str(); 

    if (rxValue.length() > 0) {
        
        Serial.print("RX: ");
        Serial.println(rxValue.c_str());

        StaticJsonDocument<JSON_DOC_SIZE> doc;
        DeserializationError error = deserializeJson(doc, rxValue.c_str());

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