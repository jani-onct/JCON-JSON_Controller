#include "JCON_BLE_PicoW.h"
#include <SPI.h>

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

    float JCON_Stick_L_x;
    float JCON_Stick_L_y;
    float JCON_Stick_R_x;
    float JCON_Stick_R_y;

    float JCON_UL;
    float JCON_UR;
};

JConData controllerData;

void parseJConData(StaticJsonDocument<JSON_DOC_SIZE> &doc, void *dataPtr)
// void parseJConData(JsonDocument &doc, void *dataPtr)
{
    JConData *data = static_cast<JConData *>(dataPtr);
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

const char *DEVICE_NAME = "JCON-PicoW";
JConBLE_PicoW jconBle;

void setup()
{
    jconBle.begin(DEVICE_NAME, &controllerData, parseJConData);
}

unsigned long lastSendTime = 0;
int testCounter = 1;
unsigned long lastDisplayTime = 0;

void loop()
{
    jconBle.loop();

    static unsigned long lastSendTime = 0;
    static int testCounter = 1;
    const unsigned long SEND_INTERVAL = 200;

    if (jconBle.isConnected())
    {
        unsigned long currentMillis = millis();

        if (currentMillis - lastSendTime >= SEND_INTERVAL)
        {
            lastSendTime = currentMillis;

            StaticJsonDocument<200> txDoc;
            txDoc["test"] = testCounter;

            String output;
            serializeJson(txDoc, output);

            jconBle.send(output);

            // Serial.print("TX (Notify): ");
            // Serial.println(output);

            testCounter++;
            if (testCounter > 10)
            {
                testCounter = 1;
            }
        }
    }

    if (JCON_newDataAvailable)
    {
        // 以下の /* */ を外すと Raw JSON が表示されます。
        /*
        Serial.print("RX (Raw JSON): ");
        Serial.println(JCON_lastRxJsonBuffer);
        */

        Serial.printf("RX Data -> Btns:[%d%d%d%d%d%d%d%d], Stick_L:(%.2f,%.2f), Stick_R:(%.2f,%.2f), UL:%.2f, UR:%.2f\n",
                      controllerData.JCON_1, controllerData.JCON_2, controllerData.JCON_3, controllerData.JCON_4,
                      controllerData.JCON_5, controllerData.JCON_6, controllerData.JCON_7, controllerData.JCON_8,
                      controllerData.JCON_Stick_L_x, controllerData.JCON_Stick_L_y,
                      controllerData.JCON_Stick_R_x, controllerData.JCON_Stick_R_y,
                      controllerData.JCON_UL, controllerData.JCON_UR);

        JCON_newDataAvailable = false;
    }
}