#include "JCON_BLE.h"

void ServerCallbacks::onConnect(BLEServer* pServer) {
    if (_pInstance) {
        _pInstance->deviceConnected = true;
        Serial.println("[EVENT] Device connected. Ready for WRITE.");
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

    } else {
        Serial.println("[WARNING] WRITE event triggered, but received 0 bytes.");
    }
}

JConBLE::JConBLE() {
}

void JConBLE::begin(const char* deviceName, void* dataPtr, ParseCallback parseCallback) {
    
    Serial.begin(115200);
    Serial.println("Starting JConBLE server...");
    
    // BLEデバイスの初期化と名前設定
    BLEDevice::init(deviceName); 
    
    // BLEサーバーの作成
    pServer = BLEDevice::createServer();
    
    // サーバーコールバックの設定
    pServerCallbacks = new ServerCallbacks(this); 
    pServer->setCallbacks(pServerCallbacks); 

    // サービスの作成
    pService = pServer->createService(SERVICE_UUID);
    
    // 特性の作成 (WRITE_NR (応答なし書き込み) を使用)
    pWriteCharacteristic = new BLECharacteristic(
                                        WRITE_CHAR_UUID,
                                        BLECharacteristic::PROPERTY_WRITE_NR
                                    );
    pService->addCharacteristic(pWriteCharacteristic); 

    // 特性コールバックのインスタンスを作成し、設定
    pWriteCallbacks = new WriteCharacteristicCallbacks(dataPtr, parseCallback);
    pWriteCharacteristic->setCallbacks(pWriteCallbacks);

    // サービスを開始
    pService->start();

    // アドバタイジングを開始
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    
    // アドバタイジング間隔の設定
    pAdvertising->setMinPreferred(0x06); 
    pAdvertising->setMaxPreferred(0x12);
    
    BLEDevice::startAdvertising();
    Serial.println("BLE Advertising started.");
}