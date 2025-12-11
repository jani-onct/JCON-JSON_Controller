#ifndef JCON_BLE_PICOW_H
#define JCON_BLE_PICOW_H

#include <BTstackLib.h>
#include <ArduinoJson.h>
#include <functional>

#define SERVICE_UUID      "2f11534c-b252-f1a8-7544-ef1f01004faf"
#define WRITE_CHAR_UUID   "2f11534c-b252-f1a8-7544-ef1f02004faf" 
#define NOTIFY_CHAR_UUID  "2f11534c-b252-f1a8-7544-ef1f03004faf" 

const size_t JSON_DOC_SIZE = 2048; 
using ParseCallback = std::function<void(StaticJsonDocument<JSON_DOC_SIZE>&, void*)>;

const size_t MAX_RX_JSON_SIZE = 512; 
extern char JCON_lastRxJsonBuffer[MAX_RX_JSON_SIZE]; 
extern volatile bool JCON_newDataAvailable;
extern volatile bool JCON_wasDisconnected;

extern "C" {
    extern void cyw43_arch_gpio_put(uint led_pin, bool value);
    extern void btstack_notify_now(hci_con_handle_t con_handle, uint16_t handle, const uint8_t *data, uint16_t len);
}

class JConBLE_PicoW {
public:
    JConBLE_PicoW();

    void begin(const char* deviceName, void* dataPtr, ParseCallback parseCallback);
    
    void loop();

    void send(String jsonString);

    bool isConnected() const;

private:
    static void staticDeviceConnectedCallback(BLEStatus status, BLEDevice *device);
    static void staticDeviceDisconnectedCallback(BLEDevice *device);
    static int staticGattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size);

    static JConBLE_PicoW* _instance;

    void* _dataPtr;
    ParseCallback _parseCallback;

    hci_con_handle_t _connection_handle = 0; 
    uint16_t _notify_value_handle = 0;
    bool _notify_enabled = false;
    bool _led_on = false;
    unsigned long _last_toggle_time = 0;

    void deviceConnectedCallback(BLEStatus status, BLEDevice *device);
    void deviceDisconnectedCallback(BLEDevice *device);
    int gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size);
};

#endif