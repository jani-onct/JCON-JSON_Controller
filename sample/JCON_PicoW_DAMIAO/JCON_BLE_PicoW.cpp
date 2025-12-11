#include "JCON_BLE_PicoW.h"
#include "ble/att_server.h"

#ifndef HCI_CON_HANDLE_INVALID
#define HCI_CON_HANDLE_INVALID 0
#endif

JConBLE_PicoW *JConBLE_PicoW::_instance = nullptr;

char JCON_lastRxJsonBuffer[MAX_RX_JSON_SIZE] = "";
volatile bool JCON_newDataAvailable = false;
volatile bool JCON_wasDisconnected = false;

#define PICOW_LED_PIN 0

extern "C"
{
    void btstack_notify_now(hci_con_handle_t con_handle, uint16_t handle, const uint8_t *data, uint16_t len)
    {
        att_server_notify(con_handle, handle, (uint8_t *)data, len);
    }
}

JConBLE_PicoW::JConBLE_PicoW()
{
    _instance = this;
}

bool JConBLE_PicoW::isConnected() const
{
    return _connection_handle != HCI_CON_HANDLE_INVALID;
}

void JConBLE_PicoW::begin(const char *deviceName, void *dataPtr, ParseCallback parseCallback)
{
    Serial.begin(115200);
    Serial.println("Starting JConBLE PicoW server...");

    _dataPtr = dataPtr;
    _parseCallback = parseCallback;

    BTstack.setBLEDeviceConnectedCallback(staticDeviceConnectedCallback);
    BTstack.setBLEDeviceDisconnectedCallback(staticDeviceDisconnectedCallback);
    BTstack.setGATTCharacteristicWrite(staticGattWriteCallback);
    BTstack.addGATTService(new UUID(SERVICE_UUID));
    BTstack.addGATTCharacteristicDynamic(
        new UUID(WRITE_CHAR_UUID),
        ATT_PROPERTY_READ | ATT_PROPERTY_WRITE | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
        0);

    _notify_value_handle = BTstack.addGATTCharacteristicDynamic(
        new UUID(NOTIFY_CHAR_UUID),
        ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY,
        0);

    BTstack.setup(deviceName);
    BTstack.startAdvertising();
    Serial.println("Advertising started.");

    cyw43_arch_gpio_put(0, false);
    _led_on = false;
    _last_toggle_time = millis();
}

void JConBLE_PicoW::staticDeviceConnectedCallback(BLEStatus status, BLEDevice *device)
{
    if (_instance && status == BLE_STATUS_OK)
    {
        _instance->deviceConnectedCallback(status, device);
    }
}

void JConBLE_PicoW::staticDeviceDisconnectedCallback(BLEDevice *device)
{
    if (_instance)
    {
        _instance->deviceDisconnectedCallback(device);
    }
}

int JConBLE_PicoW::staticGattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size)
{
    if (_instance)
    {
        return _instance->gattWriteCallback(value_handle, buffer, size);
    }
    return 0;
}

void JConBLE_PicoW::deviceConnectedCallback(BLEStatus status, BLEDevice *device)
{
    Serial.println("Device connected! LED ON.");
    _connection_handle = device->getHandle();
    cyw43_arch_gpio_put(0, true);
    _led_on = true;
}

void JConBLE_PicoW::deviceDisconnectedCallback(BLEDevice *device)
{
    Serial.println("Disconnected. Restarting advertising. LED OFF/Blinking.");
    BTstack.startAdvertising();
    _connection_handle = 0;
    _notify_enabled = false;
    cyw43_arch_gpio_put(PICOW_LED_PIN, false);
    _led_on = false;
    _last_toggle_time = millis();
    JCON_wasDisconnected = true;
}

int JConBLE_PicoW::gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size)
{
    if (_notify_value_handle != 0 && value_handle == _notify_value_handle + 1)
    {
        if (size == 2 && buffer[0] == 0x01 && buffer[1] == 0x00)
        {
            Serial.println("NOTIFY SUBSCRIBED");
            _notify_enabled = true;
        }
        else
        {
            Serial.println("NOTIFY UNSUBSCRIBED");
            _notify_enabled = false;
        }
        return 0;
    }

    if (size == 0 || size > JSON_DOC_SIZE - 1)
        return 0;

    if (size > 0 && size < MAX_RX_JSON_SIZE)
    {
        memcpy(JCON_lastRxJsonBuffer, buffer, size);
        JCON_lastRxJsonBuffer[size] = '\0';
        JCON_newDataAvailable = true;
    }

    StaticJsonDocument<JSON_DOC_SIZE> doc;
    DeserializationError error = deserializeJson(doc, JCON_lastRxJsonBuffer);

    if (error)
    {
        Serial.print(F("[ERROR] JSON parse failed: "));
        Serial.println(error.f_str());
        return 0;
    }

    if (_parseCallback && _dataPtr)
    {
        _parseCallback(doc, _dataPtr);
    }

    // Raw JSON を表示する
    // Serial.print("RX (Raw JSON): "); Serial.println(temp_buffer);

    return 0;
}

void JConBLE_PicoW::send(String jsonString)
{
    if (isConnected() && _notify_enabled && _notify_value_handle != 0)
    {
        btstack_notify_now(_connection_handle, _notify_value_handle,
                           (const uint8_t *)jsonString.c_str(), jsonString.length());
    }
}

void JConBLE_PicoW::loop()
{
    BTstack.loop();
    if (!isConnected())
    {
        const unsigned long BLINK_INTERVAL = 250;

        if (millis() - _last_toggle_time >= BLINK_INTERVAL)
        {
            _last_toggle_time = millis();

            _led_on = !_led_on;
            cyw43_arch_gpio_put(PICOW_LED_PIN, _led_on);
        }
    }
}