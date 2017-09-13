#include <avr/wdt.h>
#include <EEPROM.h>
#include <Wire.h>
#include <TroykaMQ.h>
#include <Mic.h>
#include <Volt.h>
#include <Relay.h>
#include <GPRS_Shield_Arduino.h>
#define PIN_MQ2         A0
#define PIN_MQ2_HEATER  11
#define PIN_MQ9         A1
#define PIN_MQ9_HEATER  10
#define PIN_MIC         A2
#define PIN_VOLT        A3
#define PIN_RELAY       12
#define SLEEP_DELAY     20
#define MASTER_QUERY_TIME_THRESHOLD 120
#define PROCESS_PARITY_VALUE 19

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
byte conn_error = 0;
byte modem_err = 0;
byte wakeup_reason = 0;//0-run, 1-end sleep, 2-external(HTTP), 3-external(RING)
byte process_parity = 0;
unsigned long startloopmillis = 0;
unsigned long delayms = 0;

GPRS gprs(Serial1, 4, 5);

void setup()
{
  wdt_disable();
  Wire.begin(5);
  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
  pinMode(13, OUTPUT);
  gprs.powerOff();
  digitalWrite(13, 1); delay(1000); digitalWrite(13, 0); delay(1000);
  digitalWrite(13, 1); delay(1000); digitalWrite(13, 0); delay(1000);
  digitalWrite(13, 1); delay(1000); digitalWrite(13, 0); delay(1000);
  wdt_enable(WDTO_8S);

  Serial.begin(115200);
  Serial1.begin(115200);

  pinMode(PIN_MQ2_HEATER, OUTPUT);
  pinMode(PIN_MQ9_HEATER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  mq2.heaterPwrHigh();
  mq9.cycleHeat();
}

void loop() {
  startloopmillis = millis();
  wdt_reset();
  debout("-------------------------");
  if (sleeptime >= sleep_threshold) {
    if (!relay.status()) { relay.on(); debout("relay ON"); }
    if (master_query_time >= MASTER_QUERY_TIME_THRESHOLD) { while(1) {}; }
    //if (sleeptime == 0) { master_query_time++; }/testing
    readSensors();
    debout("working");
  } else { // sleeptime < sleep_threshold
    if (relay.status()) { relay.off(); debout("relay OFF"); }
    mq2.heaterPwrOff();
    mq9.heaterPwrOff();
    if (digitalRead(5)) {
      process_parity++;
      if (process_parity >= (PROCESS_PARITY_VALUE - 1)) {
        processResp();
        if (process_parity == PROCESS_PARITY_VALUE) { process_parity = 0; }
      }
    } else {
      debout("Modem ON");
      modemOn();
    }
  }
  if (sleeptime > 0) {
    sleeptime--;
    if (sleeptime == 1) { wakeup_reason = 1; }
  } else {
    if (sleep_threshold > 0) {
      sleep_threshold = 0;
      gprs.powerOff();
      mq2.heaterPwrHigh();
      mq9.cycleHeat();
      master_query_time = 0;
    }
  }
  volt.readVolt();
  debout("analog: " + String(analogRead(A3)) + "        charge: " + String(volt.getCharge(getParam(0, 1), getParam(2, 3)), 3));//testing
  blink();
  delayms = 1000 - (millis() - startloopmillis);
  if ((delayms >= 0) && (delayms <= 1000)) { delay(delayms); } else { delay(1); }
}

void blink() {
  if (relay.status()) {
    digitalWrite(13, 1);
    delay(100);
    digitalWrite(13, 0);
  } else {
    digitalWrite(13, 0);
    delay(100);
    digitalWrite(13, 1);
  }
}

void readSensors() {
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
  debout("readSensors");
}

unsigned int getParam(unsigned int addrH, unsigned int addrL) {
  unsigned int param = EEPROM.read(addrH);
  param = param << 8;
  param = param | EEPROM.read(addrL);
  return param;
}

void receiveHandler(int bc) {
  if (Wire.available() == 3) {
    command = Wire.read();//1b//1-getValues,2-doSleep,
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
    unsigned int mic_val = mic.getNoise();
    byte data[] = {
      1,
      mq2_val_ready,
      (MQ2[0] >> 8) & 0xFF, MQ2[0] & 0xFF,
      (MQ2[1] >> 8) & 0xFF, MQ2[1] & 0xFF,
      (MQ2[2] >> 8) & 0xFF, MQ2[2] & 0xFF,
      (MQ2[3] >> 8) & 0xFF, MQ2[3] & 0xFF,
      mq9_val_ready,
      (MQ9[0] >> 8) & 0xFF, MQ9[0] & 0xFF,
      (MQ9[1] >> 8) & 0xFF, MQ9[1] & 0xFF,
      (MQ9[2] >> 8) & 0xFF, MQ9[2] & 0xFF,
      (mic_val >> 8) & 0xFF, mic_val & 0xFF,
    };
    mq2_val_ready = false;
    mq9_val_ready = false;
    Wire.write(data, sizeof(data));
  } else if (command == 2) {
    master_query_time = 0;
    if (commandValue > 0) {
      sleeptime = commandValue * 60 - 8;
      sleep_threshold = sleeptime - SLEEP_DELAY;
      Wire.write(1);
    } else {
      Wire.write(2);
    }
  } else if (command == 3) {//get only charge
    master_query_time = 0;
    unsigned int charge = round(volt.getCharge(getParam(0, 1), getParam(2, 3)) * 1000);
    byte data[] = { 1, (charge >> 8) & 0xFF, charge & 0xFF };
    Wire.write(data, sizeof(data));
  } else if (command == 4) {//request statistic
    byte data[] = { 1, wakeup_reason, conn_error, modem_err };
    wakeup_reason = 0;
    conn_error = 0;
    modem_err = 0;
    Wire.write(data, sizeof(data));
  } else if (command == 5) {//write mincharge
    EEPROM.write(0, (commandValue >> 8) & 0xFF);
    EEPROM.write(1, commandValue & 0xFF);
    Wire.write(1);
  } else if (command == 6) {//write maxcharge
    EEPROM.write(2, (commandValue >> 8) & 0xFF);
    EEPROM.write(3, commandValue & 0xFF);
    Wire.write(1);
  } else if (command == 7) {//write devid
    EEPROM.write(4, (commandValue >> 8) & 0xFF);
    EEPROM.write(5, commandValue & 0xFF);
    Wire.write(1);
  } else if (command == 8) {//write port
    EEPROM.write(6, (commandValue >> 8) & 0xFF);
    EEPROM.write(7, commandValue & 0xFF);
    Wire.write(1);
  } else if (command == 9) {//write nowakethres
    EEPROM.write(8, (commandValue >> 8) & 0xFF);
    EEPROM.write(9, commandValue & 0xFF);
    Wire.write(1);
  } else if (command == 10) {//get params
    int addrcnt = 10;
    byte data[addrcnt];
    for (int i = 0; i < addrcnt; i++) {
      data[i] = EEPROM.read(i);
    }
    Wire.write(data, sizeof(data));
  } else if (command == 11) {//heartbeat
    master_query_time = 0;
    Wire.write(1);
  } else {
    Wire.write(2);
  }
  command = 0;
  commandValue = 0;
}

void modemOn() {
  gprs.powerOn();
  wdt_reset();
  byte gprsinit_wait = 0;
  while (!gprs.init()) {
    delay(1000);
    gprsinit_wait++;
    wdt_reset();
    if (gprsinit_wait > 10) {
      gprs.powerOff();
      gprs.powerOn();
      wdt_reset();
      gprsinit_wait = 0;
    }
  }
  debout("GPRS init!");
  Serial1.println("ATE0");
  delay(1000);
  Serial1.println("AT+HTTPINIT");
  delay(3000);
  wdt_reset();
  Serial1.println("AT+SAPBR=1,1");
  delay(3000);
  wdt_reset();
  sendHTTPReq();
}

void sendHTTPReq() {
  float charge = volt.getCharge(getParam(0, 1), getParam(2, 3));
  unsigned int devid = getParam(4, 5);
  unsigned int httpport = getParam(6, 7);
  Serial1.println("AT+HTTPPARA=\"URL\",\"http://geoworks.pro:" + String(httpport) + "/heartbeat/" + String(devid) + "/ard?charge=" + String(charge, 3) + "\"");
  delay(1000);
  Serial1.println("AT+HTTPACTION=0");
  delay(1000);
  wdt_reset();
}

String getResp() {
  String input = String("                                                                ");
  byte strpos = 0;
  while (Serial1.available() > 0) {
    input[strpos] = Serial1.read();
    strpos++;
  }
  return input;
}

void processResp() {
  String tmpResp = getResp();
  if (tmpResp.indexOf(String("+HTTPACTION:")) >= 0) {
    if (tmpResp.indexOf(String(",200,")) >= 0) {
      Serial1.println("AT+HTTPREAD");
    } else {
      conn_error++;
      debout("action NO 200");
      sendHTTPReq();
    }
    debout("process 1");
  } else if (tmpResp.indexOf(String("+HTTPREAD:")) >= 0) {
    byte indFrom = tmpResp.indexOf(":1") + 4;
    char act = tmpResp.substring(indFrom, indFrom + 1)[0];
    if (act == '0') {
      conn_error = 0;
      debout("NOTHING");
    } else if (act == '1') {
      conn_error = 0;
      float charge = volt.getCharge(getParam(0, 1), getParam(2, 3));//0-1
      float wakethres = ((float)getParam(8, 9) / (float)1000);//1000 = 1
      if (charge > wakethres) {
        sleeptime = 0;
        wakeup_reason = 2;
      }
      debout("current charge " + String(charge) + " ----- wakethres " + String(wakethres));
    } else if (act == '2') {
      while(1) {};
    } else {
      conn_error++;
    }
    debout("process 2");
    sendHTTPReq();
  } else if ((tmpResp.indexOf(String("RING")) >= 0) || (tmpResp.indexOf(String("+CLIP:")) >= 0)) {
    //sleeptime = 0;
    //wakeup_reason = 3;
    Serial1.println("ATA");
    delay(1000);
    Serial1.println("ATH0");
  } else {
    conn_error++;
    debout("process ELSE");
  }
  if (conn_error > 5) {
    wdt_reset();
    debout("repeat");
    //Serial1.println("AT+SAPBR=0,1");//close
    Serial1.println("AT+SAPBR=1,1");
    delay(3000);
    Serial1.println("AT+HTTPACTION=0");
    delay(3000);
    wdt_reset();
    conn_error = 0;
    modem_err++;
  }
  if (modem_err > 3) {
    debout("modem reboot");
    gprs.powerOff();
    wdt_reset();
    modemOn();
    modem_err = 0;
  }
  debout(tmpResp);
}

void debout(char* out) {
  Serial.println(out);
}

void debout(String out) {
  Serial.println(out);
}
