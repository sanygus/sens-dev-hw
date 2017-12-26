#include <avr/wdt.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Sleep_n0m1.h>
#include "./libraries/Volt/Volt.h"
#include "./libraries/Relay/Relay.h"
#include "./libraries/GPRS-Shield/GPRS_Shield_Arduino.h"
Sleep sleep;
unsigned long sleepTime;
/*#include <TroykaMQ.h>
#include <Mic.h>
#define PIN_MQ2         A0
#define PIN_MQ2_HEATER  11
#define PIN_MQ9         A1
#define PIN_MQ9_HEATER  10
#define PIN_MIC         A2*/
#define PIN_VOLT        A3
#define PIN_RELAY       6
#define SLEEP_DELAY     20
#define MASTER_QUERY_TIME_THRESHOLD 120
#define PROCESS_PARITY_VALUE 19

Volt volt(PIN_VOLT);
Relay relay(PIN_RELAY, 13);
GPRS gprs(Serial1, 4, 5);

byte command = 0;
unsigned int commandValue = 0;
unsigned long sleeptime = 0;
unsigned long sleep_threshold = 0;
unsigned long master_query_time = 0;
byte conn_error = 0;
byte modem_err = 0;
byte wakeup_reason = 0;//0-run, 1-end sleep, 2-external(HTTP), 3-external(RING)
byte process_parity = 0;
unsigned long startloopmillis = 0;
unsigned long delaydiffms = 0;
byte delayminussleep = 0;

void setup()
{
  wdt_disable();
  Wire.begin(5);
  Wire.onReceive(receiveHandler);
  Wire.onRequest(requestHandler);
  pinMode(13, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  //При включении аврдуино gprs может быть в включенном состоянии поэтому выключаем
  gprs.powerOff();
  //просто моргаем светодиодом на плате
  digitalWrite(13, 1); delay(1000); digitalWrite(13, 0); delay(1000);
  digitalWrite(13, 1); delay(1000); digitalWrite(13, 0); delay(1000);
  digitalWrite(13, 1); delay(1000); digitalWrite(13, 0); delay(1000);
  wdt_enable(WDTO_8S);

  //сериал по usb
  Serial.begin(115200);
  //сериал по gprs
  Serial1.begin(115200);
  
}

//master_query_time если расбери что то отправила то переменная обновляется
//sleeptime == 0 активный режим
// sleeptime !=0 спим или готовимся ко сну
//sleep_threshold =sleeptime-20 сек
void loop() {
  startloopmillis = millis();
  wdt_reset();
  //debout("-------------------------");
  // пришло ли время включать расбери
  if (sleeptime >= sleep_threshold) {
    //если расбери не включен запускаем процесс включения
    if (!relay.status()) { relay.on(); debout("relay ON"); }
    //
    if (master_query_time >= MASTER_QUERY_TIME_THRESHOLD) { while(1) {}; }

    // 
    if (sleeptime == 0) { master_query_time++; }
    //readSensors();
    debout("working");
    debout("master_query_time: " + String(master_query_time));
  } else { // sleeptime < sleep_threshold
    //если расбери включен, то вырубаем его
    if (relay.status()) { relay.off(); debout("relay OFF"); }
    // если модем включен 
    if (digitalRead(5)) {

      //на 18-19 секунде выполняем processResp
      process_parity++;
      if (process_parity >= (PROCESS_PARITY_VALUE - 1)) {
        processResp();
        if (process_parity == PROCESS_PARITY_VALUE) { process_parity = 0; }
      }
    } else {
      debout("Modem ON");
      // процесс включения выключенного модема
      modemOn();
    }
  }


// 
  if (sleeptime > 0) {
    sleeptime--;
    debout("sleeptime is " + String(sleeptime));
    // время сна закончилось и причина сна в виде 1
    if (sleeptime == 0) { wakeup_reason = 1; }
  } else {
    // переход в активный режим
    if (sleep_threshold > 0) {
      sleep_threshold = 0;
      //отключаем свой модем
      gprs.powerOff();
      debout("out of sleep");
      master_query_time = 0;
    }
    
  }
  
  volt.readVolt();
  debout("analog: " + String(analogRead(A3)) + "        charge: " + String(volt.getCharge(getParam(0, 1), getParam(2, 3)), 3));//testing
  //Светодиодом показываем включен ли расбери или ардуино
  blink();
  //Дожидаемся конца секунды исходя из переменной который задается в начале loop: startloopmillis
  delayCycle();
}

void blink() {
  if (relay.status()) {
    digitalWrite(13, 1);
    delay(10);
    digitalWrite(13, 0);
  } else {
    digitalWrite(13, 0);
    delay(10);
    digitalWrite(13, 1);
  }
}

void delayCycle() {
  //вычисляем сколько милисекунд нужно еще ждать
  delaydiffms = millis() - startloopmillis;
  debout("delaydiffms " + String(delaydiffms));
  // delayDiffms может быть больше секунды?
  if ((delaydiffms > 1000) && (delaydiffms < 120000)) {
    delayminussleep = delaydiffms / 1000;
    debout("delayminussleep " + String(delayminussleep));
    if (sleeptime > delayminussleep) { sleeptime -= delayminussleep; }
    delaydiffms -= delayminussleep * 1000;
  }
  // ждем сколько нужно
  if (delaydiffms < 1000) {
    debout("delay " + String(1000 - delaydiffms));
    sleep.pwrSaveMode(); //set sleep mode
  sleep.sleepDelay(1000 - delaydiffms); //sleep for: sleepTime
  }
}


// получаем int(16 бит) переменную из EEPROM
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
  debout(String(command));
}

void requestHandler() {
if (command == 2) {
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
    debout("EEPROM.writed");
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
  debout("try init GPRS");
  while (!gprs.init()) {
    debout("IN init GPRS");
    delay(1000);
    gprsinit_wait++;
    wdt_reset();
    if (gprsinit_wait > 10) {
      debout("gprsinit_wait REBOOT");
      gprs.powerOff();
      relay.on(); delay(500); relay.off();
      wdt_reset();
      gprs.powerOn();
      wdt_reset();
      gprsinit_wait = 0;
    }
  }
  wdt_reset();
  debout("GPRS init!");
  Serial1.println("ATE0");
  delay(100);
  modemInit();
}

void modemInit() {
  sim900_send_cmd("AT+SAPBR=0,1\r\n");
  delay(500);
  while(sim900_check_with_cmd("AT+SAPBR=2,1\r\n","+SAPBR: 1,3,\"0.0.0.0\"",CMD)) {
    sim900_check_with_cmd("AT+SAPBR=1,1\r\n","OK", CMD, 20, 20);
    debout("try connect");
    wdt_reset();
    delay(2000);
  }
  debout("connected");
  delay(1000);
  sim900_send_cmd("AT+HTTPTERM\r\n");
  delay(100);
  while(!sim900_check_with_cmd("AT+HTTPINIT\r\n","OK",CMD)) {
    debout("HTTPINIT");
    wdt_reset();
    delay(1000);
  }
  debout("HTTPINITed");
  delay(1000);
  wdt_reset();
  sendHTTPReq();
}

//функция отправляет напряжение устройства на сервер через http
void sendHTTPReq() {
  //получаем напряжение
  float charge = volt.getCharge(getParam(0, 1), getParam(2, 3));
  // получаем id устройства
  unsigned int devid = getParam(4, 5);
  // получаем порт сервера
  unsigned int httpport = getParam(6, 7);
  //sim900_flush_serial();
  //отправляем данные на сервер
  Serial1.println("AT+HTTPPARA=\"URL\",\"http://geoworks.pro:" + String(httpport) + "/heartbeat/" + String(devid) + "/ard?charge=" + String(charge, 3) + "\"");
  delay(1000);
  // устанавливаем, что это GET запрос
  Serial1.println("AT+HTTPACTION=0");
  delay(1000);
  // сбрасываем таймер
  wdt_reset();
}

String getResp() {
  // выделяем строку
  String input = String("                                                                ");
  byte strpos = 0;
  // если есть что считать с сериал у модема, считаем и возвращаем
  while (Serial1.available() > 0) {
    input[strpos] = Serial1.read();
    strpos++;
  }
  return input;
}

void processResp() {
  // смотрим есть ли что от модема
  String tmpResp = getResp();
  // если модем отправил строку
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
    debout("reinit");
    modemInit();
    conn_error = 0;
    modem_err++;
    debout("reinit end");
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
