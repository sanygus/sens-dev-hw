#include <TroykaMQ.h>
#include <Mic.h>
#include <Volt.h>
#include <Relay.h>
#define PIN_MQ2         A0
#define PIN_MQ2_HEATER  11
#define PIN_MQ9         A1
#define PIN_MQ9_HEATER  10
#define PIN_MIC         A2
#define PIN_VOLT        A3
//#define PIN_AMP         A4
#define PIN_RELAY       12
#define SLEEP_DELAY     20

MQ2 mq2(PIN_MQ2, PIN_MQ2_HEATER);
MQ9 mq9(PIN_MQ9, PIN_MQ9_HEATER);
Mic mic(PIN_MIC);
Volt volt(PIN_VOLT);
Relay relay(PIN_RELAY, 13);

unsigned int MQ2[] = {0, 0, 0, 0}; // LPG (пропан-бутан сжиж), Methane (метан), Smoke (дым), Hydrogen (водород) in ppm
unsigned int MQ9[] = {0, 0, 0}; // LPG, Methane, CarbonMonoxide (угарный газ) in ppm
byte sensors[20];
boolean mq2_val_ready = false;
boolean mq9_val_ready = false;
unsigned long sleeptime = 0;
unsigned long sleep_threshold = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(PIN_MQ2_HEATER, OUTPUT);
  pinMode(PIN_MQ9_HEATER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(13, OUTPUT);
  mq2.heaterPwrHigh();
  mq9.cycleHeat();
  relay.on();
}

void loop() {
  if (sleeptime >= sleep_threshold) {
    if (!relay.status()) {
      relay.on();
    }
    // MQ-2
    if (mq2.heatingCompleted()) {
      if (!mq2.isCalibrated()) {
        mq2.calibrate();
      } else {
        MQ2[0] = mq2.readLPG();
        MQ2[1] = mq2.readMethane();
        MQ2[2] = mq2.readSmoke();
        MQ2[3] = mq2.readHydrogen();
        mq2_val_ready = true;
      }
    }
    // MQ-9
    if (mq9.atHeatCycleEnd()) {
      if (!mq9.isCalibrated()) {
        mq9.calibrate();
      } else {
        MQ9[0] = mq9.readLPG();
        MQ9[1] = mq9.readMethane();
        MQ9[2] = mq9.readCarbonMonoxide();
        mq9_val_ready = true;
      }
      mq9.cycleHeat();
    }
    // mic
    mic.readNoise();
    // volt
    volt.readVolt();
  } else { // sleeptime < sleep_threshold
    if (relay.status()) {
      relay.off(); 
    }
    mq2.heaterPwrOff();
    mq9.heaterPwrOff();
  }
  if(sleeptime > 0) {
    sleeptime--;
  } else {
    if (sleep_threshold > 0) {
      sleep_threshold = 0;
      mq2.heaterPwrHigh();
      mq9.cycleHeat();
    }
  }
  processComm();
  delay(1000);
}

void processComm() {
  if (Serial.available() == 3) {
    byte command = Serial.read();//1b//1-getValues,2-doSleep
    unsigned int commandValue = Serial.read();//2b
    commandValue = commandValue << 8;
    commandValue = commandValue | Serial.read();
    switch (command) {
      case 1:
        Serial.write(1);
        prepSensors();
        Serial.write(sensors, sizeof(sensors));
        break;
      case 2:
        if (commandValue > 0) {
          sleeptime = commandValue * 60;
          sleep_threshold = sleeptime - SLEEP_DELAY;
          Serial.write(1);
        } else {
          Serial.write(2);
        }
        break;
      default:
        Serial.write(2);
    }
  } else if (Serial.available() > 0) {
    while (Serial.available()) {
      Serial.read(); 
    }
  }
}

void prepSensors() {
  unsigned int mic_val = mic.getNoise();
  unsigned int volt_val = volt.getVolt();
  sensors[0] = mq2_val_ready;
  sensors[1] = MQ2[0] >> 8;
  sensors[2] = MQ2[0] & 0xFF;
  sensors[3] = MQ2[1] >> 8;
  sensors[4] = MQ2[1] & 0xFF;
  sensors[5] = MQ2[2] >> 8;
  sensors[6] = MQ2[2] & 0xFF;
  sensors[7] = MQ2[3] >> 8;
  sensors[8] = MQ2[3] & 0xFF;
  sensors[9] = mq9_val_ready;
  sensors[10] = MQ9[0] >> 8;
  sensors[11] = MQ9[0] & 0xFF;
  sensors[12] = MQ9[1] >> 8;
  sensors[13] = MQ9[1] & 0xFF;
  sensors[14] = MQ9[2] >> 8;
  sensors[15] = MQ9[2] & 0xFF;
  sensors[16] = mic_val >> 8;
  sensors[17] = mic_val & 0xFF;
  sensors[18] = volt_val >> 8;
  sensors[19] = volt_val & 0xFF;
  mq2_val_ready = false;
  mq9_val_ready = false;
}
