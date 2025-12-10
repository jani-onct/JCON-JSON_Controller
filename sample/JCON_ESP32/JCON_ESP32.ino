#include "JCON_BLE.h"

struct JConData
{
    int JCON_1;
    int JCON_2;
    int JCON_3;
    int JCON_4;
    int JCON_5;
    int JCON_6;
    int JCON_7;
    int JCON_8;
    int JCON_D_Pad;

    float JCON_L_X;
    float JCON_L_Y;
    float JCON_R_X;
    float JCON_R_Y;

    float JCON_UL;
    float JCON_UR;
};
JConData controllerData;

void parseJConData(StaticJsonDocument<JSON_DOC_SIZE> &doc, void *dataPtr)
{
    JConData *data = static_cast<JConData *>(dataPtr);
    data->JCON_1 = doc["buttons"]["1"] | 0;
    data->JCON_2 = doc["buttons"]["2"] | 0;
    data->JCON_3 = doc["buttons"]["3"] | 0;
    data->JCON_4 = doc["buttons"]["4"] | 0;
    data->JCON_5 = doc["buttons"]["5"] | 0;
    data->JCON_6 = doc["buttons"]["6"] | 0;
    data->JCON_7 = doc["buttons"]["7"] | 0;
    data->JCON_8 = doc["buttons"]["8"] | 0;
    data->JCON_D_Pad = doc["buttons"]["D-Pad"] | 0;
    data->JCON_L_X = doc["axes"]["L_X"] | 0.0f;
    data->JCON_L_Y = doc["axes"]["L_Y"] | 0.0f;
    data->JCON_R_X = doc["axes"]["R_X"] | 0.0f;
    data->JCON_R_Y = doc["axes"]["R_Y"] | 0.0f;
    data->JCON_UL = doc["axes"]["UL"] | 0.0f;
    data->JCON_UR = doc["axes"]["UR"] | 0.0f;
}

const char *DEVICE_NAME = "JCON-ESP32";
JConBLE jconBle;

void setup()
{
    jconBle.begin(DEVICE_NAME, &controllerData, parseJConData);
}

// 送信用変数
unsigned long lastSendTime = 0;
int testCounter = 1;
unsigned long lastDisplayTime = 0;

// void loop() {

//     if (jconBle.isConnected()) {

//         unsigned long currentMillis = millis();
//         if (currentMillis - lastSendTime >= 200) {
//             lastSendTime = currentMillis;

//             StaticJsonDocument<200> txDoc;
//             txDoc["test"] = testCounter;

//             String output;
//             serializeJson(txDoc, output);

//             jconBle.send(output);
//             Serial.print("TX: ");
//             Serial.println(output);

//             testCounter++;
//             if (testCounter > 10) {
//                 testCounter = 1;
//             }
//         }

//         delay(10);
//     } else {
//         delay(500);
//     }
// }
void loop()
{

    if (jconBle.isConnected())
    {

        unsigned long currentMillis = millis();

        // 送信ロジック (TX) - 200msごと
        if (currentMillis - lastSendTime >= 1000)
        {
            lastSendTime = currentMillis;

            StaticJsonDocument<200> txDoc;
            txDoc["test"] = testCounter;

            String output;
            serializeJson(txDoc, output);

            jconBle.send(output);
            Serial.print("TX: ");
            Serial.println(output);

            testCounter++;
            if (testCounter > 10)
            {
                testCounter = 1;
            }
        }

        // 受信データ表示ロジック (RX) - 500msごと
        const unsigned long DISPLAY_INTERVAL = 50;
        if (currentMillis - lastDisplayTime >= DISPLAY_INTERVAL)
        {
            lastDisplayTime = currentMillis;

            // 受信した構造体の内容を出力
            Serial.println("--- JCON Data Received ---");
            // ボタン/D-Pad
            Serial.printf("Btns: 1=%d, 2=%d, 3=%d, 4=%d, 5=%d, 6=%d, 7=%d, 8=%d, D-Pad=%d\n",
                          controllerData.JCON_1, controllerData.JCON_2, controllerData.JCON_3, controllerData.JCON_4,
                          controllerData.JCON_5, controllerData.JCON_6, controllerData.JCON_7, controllerData.JCON_8,
                          controllerData.JCON_D_Pad);

            // スティック/スライダー (JConDataの定義に合わせて表示)
            Serial.printf("Stick_R: (%.2f, %.2f)\n", controllerData.JCON_R_X, controllerData.JCON_R_Y);
            Serial.printf("UL: %.2f, UR: %.2f\n", controllerData.JCON_UL, controllerData.JCON_UR);
            Serial.println("--------------------------");
        }

        delay(10);
    }
    else
    {
        delay(500);
    }
}