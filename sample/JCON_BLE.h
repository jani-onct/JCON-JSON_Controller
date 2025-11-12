#ifndef JCON_BLE_H
#define JCON_BLE_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h> 
#include <functional> 
#include <string>

// 内部で使用する固定のUUID
#define SERVICE_UUID        "4FAF0001-1FEF-4475-A8F1-52B24C53112F"
#define WRITE_CHAR_UUID     "4FAF0002-1FEF-4475-A8F1-52B24C53112F"

// JSONパース用の静的バッファサイズ
const size_t JSON_DOC_SIZE = 1024; 

// 外部定義のデータ構造へのパース処理を外部に渡すための型エイリアス
using ParseCallback = std::function<void(StaticJsonDocument<JSON_DOC_SIZE>&, void*)>;

// JConBLE クラスを前方宣言
class JConBLE; 

class ServerCallbacks: public BLEServerCallbacks {
public:
    ServerCallbacks(JConBLE* pInstance) : _pInstance(pInstance) {}

    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

private:
    JConBLE* _pInstance;
};

class WriteCharacteristicCallbacks: public BLECharacteristicCallbacks {
public:
    WriteCharacteristicCallbacks(void* dataPtr, ParseCallback callback) 
      : _dataPtr(dataPtr), _parseCallback(callback) {}

    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    void* _dataPtr;
    ParseCallback _parseCallback;
};

class JConBLE {
public:
    JConBLE();
    
    void begin(const char* deviceName, void* dataPtr, ParseCallback parseCallback);

    bool isConnected() const { return deviceConnected; }

private:
    bool deviceConnected = false;
    
    BLEServer* pServer = NULL;
    BLEService* pService = NULL; 
    BLECharacteristic* pWriteCharacteristic = NULL;
    
    ServerCallbacks* pServerCallbacks = NULL; 
    WriteCharacteristicCallbacks* pWriteCallbacks = NULL;

    friend class ServerCallbacks; 
};

#endif