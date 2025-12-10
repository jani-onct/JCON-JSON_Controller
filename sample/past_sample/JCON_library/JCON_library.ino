#include "JCON_BLE.h" 

struct JConData { // 自分で設定したキー配置などに合わせて設定
    int JCON_1;
    int JCON_2;
    int JCON_3;
    int JCON_4;
    int JCON_5;
    int JCON_6;
    int JCON_7;
    int JCON_8;
    int JCON_D_Pad;

    float JCON_R_X; 
    float JCON_R_Y; 

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

    data->JCON_R_X     = doc["axes"]["R_X"] | 0.0f; 
    data->JCON_R_Y     = doc["axes"]["R_Y"] | 0.0f; 
    
    data->JCON_UL      = doc["axes"]["UL"] | 0.0f;
    data->JCON_UR      = doc["axes"]["UR"] | 0.0f;
}

const char* DEVICE_NAME = "JCON-ESP32"; 
// デバイス名を設定
// ここで決めたデバイス名がアプリ内でスキャンした時に表示される
// デバイス名は必ず"JCON-"ではじめること
JConBLE jconBle;

void setup() {
    jconBle.begin(DEVICE_NAME, &controllerData, parseJConData);
    Serial.begin(115200);
}

void loop() {
    
static float last_R_x = 0.0f; 
    
    if (jconBle.isConnected()) {
        Serial.println(controllerData.JCON_R_X);
        delay(50); 
    } else {
        delay(1);
    }
}