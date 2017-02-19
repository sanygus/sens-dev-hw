#include <avr/wdt.h>
#include <TroykaMQ.h>
#include <Mic.h>
#include <Volt.h>
#include <Relay.h>
#include <Wire.h>
#define PIN_MQ2         A0
#define PIN_MQ2_HEATER  11
#define PIN_MQ9         A1
#define PIN_MQ9_HEATER  10
#define PIN_MIC         A2
#define PIN_VOLT        A3
#define PIN_RELAY       12
#define SLEEP_DELAY     20
#define MASTER_QUERY_TIME_THRESHOLD 120

MQ2 mq2(PIN_MQ2, PIN_MQ2_HEATER);
MQ9 mq9(PIN_MQ9, PIN_MQ9_HEATER);
Mic mic(PIN_MIC);
Volt volt(PIN_VOLT);
Relay relay(PIN_RELAY, 13);

byte command = 0;
unsigned int commandValue = 0;
unsigned int MQ2[] = {0, 0, 0, 0}; // LPG (пропан-бутан сжиж), Methane (метан), Smoke (дым), Hydrogen (водород) in ppm
unsigned int MQ9[] = {0, 0, 0}; // LPG, Methane, CarbonMonoxide (угарный газ) in ppm
boolean mq2_val_ready = false;
boolean mq9_val_ready = false;
unsigned long sleeptime = 0;
unsigned long sleep_threshold = 0;
unsigned long master_query_time = 0;

void setup()
{
  wdt_disable();
  Wire.begin(5);
  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
  pinMode(13, OUTPUT);
  digitalWrite(13, 1);
  delay(1000);
  digitalWrite(13, 0);
  delay(1000);
  digitalWrite(13, 1);
  delay(1000);
  digitalWrite(13, 0);
  delay(1000);
  digitalWrite(13, 1);
  delay(1000);
  digitalWrite(13, 0);
  delay(1000);
  wdt_enable(WDTO_4S);
  //Serial.begin(9600);
  pinMode(PIN_MQ2_HEATER, OUTPUT);
  pinMode(PIN_MQ9_HEATER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  mq2.heaterPwrHigh();
  mq9.cycleHeat();
}

void loop() {
  wdt_reset();
  if (sleeptime >= sleep_threshold) {
    if (!relay.status()) {
      relay.on();
    }
    if (master_query_time >= MASTER_QUERY_TIME_THRESHOLD) {
      while(1) {}
    }
    if (sleeptime == 0) {
      master_query_time++;
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
      /*mq2.heaterPwrHigh();
      mq9.cycleHeat();*/
      while(1) {}
    }
  }
  delay(1000);
}

void receiveHandler(int bc) {
  if (Wire.available() == 3) {
    master_query_time = 0;
    command = Wire.read();//1b//1-getValues,2-doSleep
    commandValue = Wire.read();//2b
    commandValue = commandValue << 8;
    commandValue = commandValue | Wire.read();
  } else {
    while (Wire.available()) {
      Wire.read();
    }
  }
}

void requestHandler() {
  if (command == 1) {
    master_query_time = 0;
    Wire.write(1);
    unsigned int mic_val = mic.getNoise();
    unsigned int volt_val = volt.getVolt();
    byte data[] = {
      mq2_val_ready,
      MQ2[0] >> 8, MQ2[0] & 0xFF,
      MQ2[1] >> 8, MQ2[1] & 0xFF,
      MQ2[2] >> 8, MQ2[2] & 0xFF,
      MQ2[3] >> 8, MQ2[3] & 0xFF,
      mq9_val_ready,
      MQ9[0] >> 8, MQ9[0] & 0xFF,
      MQ9[1] >> 8, MQ9[1] & 0xFF,
      MQ9[2] >> 8, MQ9[2] & 0xFF,
      mic_val >> 8, mic_val & 0xFF,
      volt_val >> 8, volt_val & 0xFF,
    };
    mq2_val_ready = false;
    mq9_val_ready = false;
    Wire.write(data, sizeof(data));
  } else if (command == 2) {
    master_query_time = 0;
    if (commandValue > 0) {
      sleeptime = commandValue * 60;
      sleep_threshold = sleeptime - SLEEP_DELAY;
      Wire.write(1);
    } else {
      Wire.write(2);
    }
  } else {
    Wire.write(2);
  }
  command = 0;
  commandValue = 0;
}
