#include "JCON_BLE_PicoW.h"
#include <SPI.h>

#include <RP2040PIO_CAN.h>  //https://github.com/eyr1n/RP2040PIO_CAN
const uint32_t CAN_TX_PIN = 0;  // 連続してなくてもいい
const uint32_t CAN_RX_PIN = 1;  // GP1とGP3みたいな組み合わせでも動く

#include <DAMIAO_Control.h>  // https://github.com/Suzu-Gears/DAMIAO_Control_Arduino/tree/main
#include <DMUtils.h>

using namespace damiao;

const uint32_t MASTER_ID = 0x00;  // モーターのMasterID
const uint32_t SLAVE_ID = 0x09;   // モーターのSlaveID
// =============================================

// モーターオブジェクトの作成
Motor motor1(MASTER_ID, SLAVE_ID);

// モーターのフィードバック情報をシリアルモニタに表示する関数
void printFeedback(Motor &motor) {
  Status status = motor.getStatus();
  Mode mode = motor.getMode();
  float pos = motor.getPosition();
  float vel = motor.getVelocity();
  float tau = motor.getTorque();

  Serial.print("[FB] ID:");
  Serial.print(motor.getSlaveId());
  Serial.print(" Status:");
  Serial.print(statusToString(status));  // ステータスを文字列に変換して表示
  Serial.print(" Mode:");
  Serial.print(modeToString(mode));  // モードを文字列に変換して表示
  Serial.print(" Pos:");
  Serial.print(pos, 2);
  Serial.print(" Vel:");
  Serial.print(vel, 2);
  Serial.print(" Tau:");
  Serial.println(tau, 2);
}

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
// void parseJConData(JsonDocument &doc, void *dataPtr)
{
    JConData *data = static_cast<JConData *>(dataPtr);
    data->JCON_1 = doc["buttons"]["1"].as<int>() | 0;
    data->JCON_2 = doc["buttons"]["2"].as<int>() | 0;
    data->JCON_3 = doc["buttons"]["3"].as<int>() | 0;
    data->JCON_4 = doc["buttons"]["4"].as<int>() | 0;
    data->JCON_5 = doc["buttons"]["5"].as<int>() | 0;
    data->JCON_6 = doc["buttons"]["6"].as<int>() | 0;
    data->JCON_7 = doc["buttons"]["7"].as<int>() | 0;
    data->JCON_8 = doc["buttons"]["8"].as<int>() | 0;
    data->JCON_D_Pad = doc["D-Pad"].as<int>() | 0;

    data->JCON_L_X = doc["axes"]["L_X"].as<float>();
    data->JCON_L_Y = doc["axes"]["L_Y"].as<float>();
    data->JCON_R_X = doc["axes"]["R_X"].as<float>();
    data->JCON_R_Y = doc["axes"]["R_Y"].as<float>();

    data->JCON_UL = doc["axes"]["UL"].as<float>();
    data->JCON_UR = doc["axes"]["UR"].as<float>();
}

const char *DEVICE_NAME = "JCON-PicoW";
JConBLE_PicoW jconBle;

void setup() {
  jconBle.begin(DEVICE_NAME, &controllerData, parseJConData);
  bool can_ok = false;
  CAN.setTX(CAN_TX_PIN);
  CAN.setRX(CAN_RX_PIN);
  can_ok = CAN.begin(CanBitRate::BR_1000k);

  if (can_ok) {
    Serial.println("CAN bus initialized successfully.");
  } else {
    Serial.println("CAN bus initialization failed!");
    while (1)
      ;  // CAN通信が開始できない場合はここで停止
  }
  motor1.setCAN(&CAN);  // モーターにCANインスタンスを渡す
  motor1.disable();     // 念のためモーターを無効化

  // 2. モーターの各種パラメータを取得
  motor1.initialize();

  // 3. モーターの制御モードを設定
  Serial.print("Setting control mode to MIT...");
  if (motor1.setControlMode(Mode::MIT)) {
    Serial.println("Success.");
  } else {
    Serial.println("Failed.");
  }

  // 4. 現在位置をゼロ点として設定
  Serial.print("Setting current position as zero...");
  if (motor1.setZeroPosition()) {
    Serial.println("Success.");
  } else {
    Serial.println("Failed.");
  }

  // 5. モーターを有効化（起動）
  Serial.print("Enabling motor...");
  if (motor1.enable()) {
    Serial.println("Success.");
  } else {
    Serial.println("Failed.");
  }
}

unsigned long lastSendTime = 0;
int testCounter = 1;
unsigned long lastDisplayTime = 0;
static bool motor_flag = 1;

void loop() {
  jconBle.loop();
  if (motor_flag == 0) {
    motor1.enable();
    motor_flag = 1;
  }
  if (JCON_wasDisconnected) {
    Serial.println("!!! CONNECTION LOST !!! Performing necessary cleanup.");
    motor1.disable();
    motor_flag = 0;
    JCON_wasDisconnected = false;
  }
  motor1.update();

  // static unsigned long lastSendTime = 0;
  // static int testCounter = 1;
  const unsigned long SEND_INTERVAL = 1000;

  if (jconBle.isConnected()) {
    unsigned long currentMillis = millis();

    if (currentMillis - lastSendTime >= SEND_INTERVAL) {
      lastSendTime = currentMillis;

      StaticJsonDocument<200> txDoc;
      txDoc["test"] = testCounter;

      String output;
      serializeJson(txDoc, output);

      jconBle.send(output);

      // Serial.print("TX (Notify): ");
      // Serial.println(output);

      testCounter++;
      if (testCounter > 10) {
        testCounter = 1;
      }
    }
    // Serial.println("hoge");
    if (JCON_newDataAvailable) {
      // 以下の /* */ を外すと Raw JSON が表示されます。
      /*
              Serial.print("RX (Raw JSON): ");
              Serial.println(JCON_lastRxJsonBuffer);
              */

      //   Serial.printf("RX Data -> Btns:[%d%d%d%d%d%d%d%d], Stick_L:(%.2f,%.2f), Stick_R:(%.2f,%.2f), UL:%.2f, UR:%.2f\n",
      //                 controllerData.JCON_1, controllerData.JCON_2, controllerData.JCON_3, controllerData.JCON_4,
      //                 controllerData.JCON_5, controllerData.JCON_6, controllerData.JCON_7, controllerData.JCON_8,
      //                 controllerData.JCON_L_X, controllerData.JCON_L_Y,
      //                 controllerData.JCON_R_X, controllerData.JCON_R_Y,
      //                 controllerData.JCON_UL, controllerData.JCON_UR);

      // 6. MITモードで指令を送信（目標位置0速度0でバネダンパの挙動）
      motor1.sendMIT(0.0f, controllerData.JCON_R_Y, 0.0f, 1.0f, 0.0f);

      const unsigned long FEEDBACK_INTERVAL_MS = 100UL;  // 100msごとに表示
      static unsigned long lastFeedbackMillis = 0;
      unsigned long now = millis();
      if (now - lastFeedbackMillis >= FEEDBACK_INTERVAL_MS) {
        lastFeedbackMillis = now;
        printFeedback(motor1);
      }

      JCON_newDataAvailable = false;
      Serial.println("if_end");
    }
  }
}