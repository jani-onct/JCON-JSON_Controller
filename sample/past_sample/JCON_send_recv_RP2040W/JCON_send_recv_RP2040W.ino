#include <BTstackLib.h>
#include <SPI.h>
#include <ArduinoJson.h> 

#include "ble/att_server.h"

#define SERVICE_UUID "2f11534c-b252-f1a8-7544-ef1f01004faf"
#define WRITE_CHAR_UUID "2f11534c-b252-f1a8-7544-ef1f02004faf"
#define NOTIFY_CHAR_UUID "2f11534c-b252-f1a8-7544-ef1f03004faf"

const size_t JSON_DOC_SIZE = 2048; 

extern "C" {
  void btstack_notify_now(hci_con_handle_t con_handle, uint16_t handle, const uint8_t *data, uint16_t len);
}

struct JConData {
  int JCON_1;
  int JCON_2;
  int JCON_3;
  int JCON_4;
  int JCON_5;
  int JCON_6;
  int JCON_7;
  int JCON_8;
  int JCON_D_Pad;

  float JCON_Stick_L_x;  
  float JCON_Stick_L_y;  
  float JCON_Stick_R_x;
  float JCON_Stick_R_y;

  float JCON_UL;
  float JCON_UR;
};
JConData controllerData;

uint16_t notify_value_handle = 0;
hci_con_handle_t connection_handle = 0;
bool notify_enabled = false;

void deviceConnectedCallback(BLEStatus status, BLEDevice *device);
void deviceDisconnectedCallback(BLEDevice *device);
uint16_t gattReadCallback(uint16_t value_handle, uint8_t *buffer, uint16_t buffer_size);
int gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size);

void parseJConData(StaticJsonDocument<JSON_DOC_SIZE> &doc, JConData *data) {

  data->JCON_1 = doc["1"].as<int>() | 0;
  data->JCON_2 = doc["2"].as<int>() | 0;
  data->JCON_3 = doc["3"].as<int>() | 0;
  data->JCON_4 = doc["4"].as<int>() | 0;
  data->JCON_5 = doc["5"].as<int>() | 0;
  data->JCON_6 = doc["6"].as<int>() | 0;
  data->JCON_7 = doc["7"].as<int>() | 0;
  data->JCON_8 = doc["8"].as<int>() | 0;
  data->JCON_D_Pad = doc["D-Pad"].as<int>() | 0;

  data->JCON_Stick_L_x = doc["Stick_L"]["x"].as<float>();
  data->JCON_Stick_L_y = doc["Stick_L"]["y"].as<float>();
  data->JCON_Stick_R_x = doc["Stick_R"]["x"].as<float>();
  data->JCON_Stick_R_y = doc["Stick_R"]["y"].as<float>();

  data->JCON_UL = doc["UL"].as<float>();
  data->JCON_UR = doc["UR"].as<float>();
}

bool led_on = false; 

extern "C" {
    extern void cyw43_arch_gpio_put(int led_pin, bool value);
}

bool isConnected() {
    return connection_handle != 0; 
}

void setup(void) {
  Serial.begin(115200); 
  Serial.println("Starting BLE Peripheral...");

  BTstack.setBLEDeviceConnectedCallback(deviceConnectedCallback);
  BTstack.setBLEDeviceDisconnectedCallback(deviceDisconnectedCallback);
  BTstack.setGATTCharacteristicRead(gattReadCallback);
  BTstack.setGATTCharacteristicWrite(gattWriteCallback);

  BTstack.addGATTService(new UUID(SERVICE_UUID));
  BTstack.addGATTCharacteristicDynamic(
    new UUID(WRITE_CHAR_UUID),
    ATT_PROPERTY_WRITE, 
    0);
  notify_value_handle = BTstack.addGATTCharacteristicDynamic(new UUID(NOTIFY_CHAR_UUID), ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY, 0);

  BTstack.setup("JCON-PicoW"); 
  BTstack.startAdvertising();
  Serial.println("Advertising started.");
}

void loop(void) {
  BTstack.loop();

  static unsigned long lastSendTime = 0;
  static int testCounter = 1;
  const unsigned long SEND_INTERVAL = 200;

  unsigned long currentMillis = millis();

  if (currentMillis - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = currentMillis;

    if (notify_value_handle != 0 && notify_enabled) {
      StaticJsonDocument<200> txDoc;
      txDoc["test"] = testCounter;

      String output;
      serializeJson(txDoc, output);
      btstack_notify_now(connection_handle, notify_value_handle, (const uint8_t *)output.c_str(), output.length());

      Serial.print("TX (Notify): ");
      Serial.println(output);

      testCounter++;
      if (testCounter > 10) {
        testCounter = 1;
      }
    }
  }
  if (!isConnected()) {
        static unsigned long lastToggleTime = 0;
        const unsigned long BLINK_INTERVAL = 250;

        if (millis() - lastToggleTime >= BLINK_INTERVAL) {
            lastToggleTime = millis();
            led_on = !led_on; 
            cyw43_arch_gpio_put(0, led_on); 
        }
    }
}

int gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size) {
    if (notify_value_handle != 0 && value_handle == notify_value_handle + 1) { 
        if (size == 2 && buffer[0] == 0x01 && buffer[1] == 0x00) {
            Serial.println("!!! NOTIFY SUBSCRIBED !!!");
            notify_enabled = true;
        } else {
            Serial.println("!!! NOTIFY UNSUBSCRIBED !!!");
            notify_enabled = false;
        }
        return 0;
    }

    if (size == 0 || size > JSON_DOC_SIZE - 1) {
        return 0; 
    }

    char temp_buffer[size + 1];
    memcpy(temp_buffer, buffer, size);
    temp_buffer[size] = '\0'; 
    
    // Raw JSONを表示する場合
    /*
    Serial.print("RX (Raw JSON): ");
    Serial.println(temp_buffer);
    */
    
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    DeserializationError error = deserializeJson(doc, temp_buffer); 

    if (error) {
        Serial.print(F("[ERROR] JSON parse failed: "));
        Serial.println(error.f_str());
        return 0; 
    }

    // 変数の内容を表示する場合
    parseJConData(doc, &controllerData);
    
    Serial.printf("RX Data -> Btns:[%d%d%d%d%d%d%d%d], Stick_L:(%.2f,%.2f), Stick_R:(%.2f,%.2f), UL:%.2f, UR:%.2f\n",
        controllerData.JCON_1, controllerData.JCON_2, controllerData.JCON_2, controllerData.JCON_4,
        controllerData.JCON_5, controllerData.JCON_6, controllerData.JCON_7, controllerData.JCON_8, 
        controllerData.JCON_Stick_L_x, controllerData.JCON_Stick_L_y,
        controllerData.JCON_Stick_R_x, controllerData.JCON_Stick_R_y,
        controllerData.JCON_UL, controllerData.JCON_UR);
    
    return 0;
}

void deviceConnectedCallback(BLEStatus status, BLEDevice *device) {
  (void)device;
  if (status == BLE_STATUS_OK) {
    Serial.println("Device connected!");
    connection_handle = device->getHandle();
    cyw43_arch_gpio_put(0, true);
        led_on = true;
  }
}

void deviceDisconnectedCallback(BLEDevice *device) {
  (void)device;
  Serial.println("Disconnected. Restarting advertising...");
  BTstack.startAdvertising();
  connection_handle = 0;
    cyw43_arch_gpio_put(0, false);
    led_on = false;
    notify_enabled = false;
}

uint16_t gattReadCallback(uint16_t value_handle, uint8_t *buffer, uint16_t buffer_size) {
  const char *read_msg = "PicoW Ready";
  uint16_t len = strlen(read_msg);

  if (buffer && buffer_size >= len) {
    memcpy(buffer, read_msg, len);
  }
  return len;
}

extern "C" {
  void btstack_notify_now(hci_con_handle_t con_handle, uint16_t handle, const uint8_t *data, uint16_t len) {
    att_server_notify(con_handle, handle, (uint8_t *)data, len);
  }
}