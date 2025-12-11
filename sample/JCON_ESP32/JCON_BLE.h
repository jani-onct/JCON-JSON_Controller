#ifndef JCON_BLE_H
#define JCON_BLE_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h> 
#include <ArduinoJson.h> 
#include <functional> 
#include <string>

#define SERVICE_UUID        "2f11534c-b252-f1a8-7544-ef1f01004faf"
#define WRITE_CHAR_UUID     "2f11534c-b252-f1a8-7544-ef1f02004faf"
#define NOTIFY_CHAR_UUID    "2f11534c-b252-f1a8-7544-ef1f03004faf" 

const size_t JSON_DOC_SIZE = 1024; 

using ParseCallback = std::function<void(StaticJsonDocument<JSON_DOC_SIZE>&, void*)>;

class JConBLE; 

class ServerCallbacks: public BLEServerCallbacks {
public:
    ServerCallbacks(JConBLE* pInstance) : _pInstance(pInstance) {}

    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) override;
    
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
    
    void send(String jsonString);

    bool isConnected() const { return deviceConnected; }

private:
    bool deviceConnected = false;
    
    BLEServer* pServer = NULL;
    BLEService* pService = NULL; 
    BLECharacteristic* pWriteCharacteristic = NULL;
    BLECharacteristic* pNotifyCharacteristic = NULL; 
    
    ServerCallbacks* pServerCallbacks = NULL; 
    WriteCharacteristicCallbacks* pWriteCallbacks = NULL;

    friend class ServerCallbacks; 
};

#endif