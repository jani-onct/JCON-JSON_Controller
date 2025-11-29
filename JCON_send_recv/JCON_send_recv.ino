#include "JCON_BLE.h" 

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

    float JCON_Stick_R_x; 
    float JCON_Stick_R_y; 

    float JCON_UL;
    float JCON_UR;
};

JConData controllerData;

void parseJConData(StaticJsonDocument<JSON_DOC_SIZE>& doc, void* dataPtr) {
    JConData* data = static_cast<JConData*>(dataPtr);
    data->JCON_1       = doc["1"] | 0;
    data->JCON_2       = doc["2"] | 0;
    data->JCON_3       = doc["3"] | 0;
    data->JCON_4       = doc["4"] | 0;
    data->JCON_5       = doc["5"] | 0;
    data->JCON_6       = doc["6"] | 0;
    data->JCON_7       = doc["7"] | 0;
    data->JCON_8       = doc["8"] | 0;
    data->JCON_D_Pad   = doc["D-Pad"] | 0;
    data->JCON_Stick_R_x     = doc["Stick_R"]["x"] | 0.0f; 
    data->JCON_Stick_R_y     = doc["Stick_R"]["y"] | 0.0f;
    data->JCON_UL      = doc["UL"] | 0.0f;
    data->JCON_UR      = doc["UR"] | 0.0f;
}

const char* DEVICE_NAME = "JCON-ESP32"; 
JConBLE jconBle;

void setup() {
    jconBle.begin(DEVICE_NAME, &controllerData, parseJConData);
}

// 送信用変数
unsigned long lastSendTime = 0;
int testCounter = 1;

void loop() {
    
    if (jconBle.isConnected()) {
        
        unsigned long currentMillis = millis();
        if (currentMillis - lastSendTime >= 200) {
            lastSendTime = currentMillis;

            StaticJsonDocument<200> txDoc;
            txDoc["test"] = testCounter;

            String output;
            serializeJson(txDoc, output);

            jconBle.send(output);
            Serial.print("TX: ");
            Serial.println(output);

            testCounter++;
            if (testCounter > 10) {
                testCounter = 1;
            }
        }
        
        delay(10); 
    } else {
        delay(500); 
    }
}