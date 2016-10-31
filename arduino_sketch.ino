#include <Wire.h>
#include <TroykaMQ.h>
#include <Mic.h>
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
Relay relay(PIN_RELAY, 13);

unsigned int MQ2[] = {0, 0, 0, 0};//LPG (пропан-бутан сжиж), Methane (метан), Smoke (дым), Hydrogen (водород) in ppm
unsigned int MQ9[] = {0, 0, 0};//LPG, Methane, CarbonMonoxide (угарный газ) in ppm
boolean mq2_val_ready = false;
boolean mq9_val_ready = false;
unsigned int volt = 0;
//unsigned int amp = 0;
unsigned int sleeptime = 0;
unsigned int sleep_threshold = 0;

void setup()
{
  //Serial.begin(9600);
  Wire.begin(0x05);
  Wire.onReceive(rec);
  Wire.onRequest(req);
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
    //MQ-2
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
    //MQ-9
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
    //mic
    mic.readNoise();
    //volt
    volt = analogRead(PIN_VOLT);
    //amp
    //amp = analogRead(PIN_AMP);
  } else { //sleeptime < sleep_threshold
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
    }
  }
  delay(1000);
}

void rec(int bc) { //receive
   if(bc==3){ //ok
     byte data[3] = {0, 0, 0}; //1 byte - command, 2,3 - value
     byte tempi = 0;
     while(Wire.available()) {
       data[tempi] = Wire.read();
       tempi++;
     }
     if(data[0]==1) {//sleep
       sleeptime = data[1] << 8 | data[2];
       sleep_threshold = sleeptime - SLEEP_DELAY;
     }
   } else { //flush buffer
     while(Wire.available()) {
       Wire.read();
     }
   }
}

void req() {
  unsigned int mic_val = mic.getNoise();
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
    volt >> 8, volt & 0xFF,
    //amp >> 8, amp & 0xFF,
  };
  mq2_val_ready = false;
  mq9_val_ready = false;
  Wire.write(data, sizeof(data));
}
