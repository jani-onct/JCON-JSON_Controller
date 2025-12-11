// JCONアプリとESP32で送受信するためのサンプルプログラムです．
// JconDataで定義したデータ構造体に受信データが格納されます．
// データ構造体のメンバ名はの"JCON_"に続く部分はJCONアプリで設定したキーと一致させてください．
// 1000ミリ秒ごとにテストデータをJCONアプリに送信します．
// ボタン，D-Padの値はint型，スティック，スライダーの値はfloat型で受信されます．

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

volatile bool newDataAvailable = false;

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

    newDataAvailable = true;
}

const char *DEVICE_NAME = "JCON-ESP32";
JConBLE jconBle;

void setup()
{
    Serial.begin(115200); 

    jconBle.begin(DEVICE_NAME, &controllerData, parseJConData);
}

unsigned long lastSendTime = 0;
unsigned long lastLogTime = 0;
int testCounter = 1;

void loop()
{
    if (jconBle.isConnected())
    {
        unsigned long currentMillis = millis();

        if (currentMillis - lastSendTime >= 1000)
        {
            lastSendTime = currentMillis;
            StaticJsonDocument<200> txDoc;
            txDoc["test"] = testCounter;

            String output;
            serializeJson(txDoc, output);

            jconBle.send(output);
            
            // Serial.print("TX: ");
            // Serial.println(output);

            testCounter++;
            if (testCounter > 10)
            {
                testCounter = 1;
            }
        }

        if (newDataAvailable && (currentMillis - lastLogTime > 30)) 
        {
            newDataAvailable = false;
            lastLogTime = currentMillis;

            Serial.printf("RX Data -> Btns:[%d%d%d%d%d%d%d%d], Stick_L:(%.2f,%.2f), Stick_R:(%.2f,%.2f), UL:%.2f, UR:%.2f\n",
                controllerData.JCON_1, controllerData.JCON_2, controllerData.JCON_3, controllerData.JCON_4,
                controllerData.JCON_5, controllerData.JCON_6, controllerData.JCON_7, controllerData.JCON_8,
                controllerData.JCON_L_X, controllerData.JCON_L_Y,
                controllerData.JCON_R_X, controllerData.JCON_R_Y,
                controllerData.JCON_UL, controllerData.JCON_UR
            );
        }
        else if (newDataAvailable) 
        {
             newDataAvailable = false; 
        }

        delay(1);
    }
    else
    {
        delay(500);
    }
}