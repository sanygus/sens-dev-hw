#ifndef Relay_h
#define Relay_h

#include "Arduino.h"

class Relay
{
  public:
    Relay(int pin_r, int pin_led);
    void on();
    void off();
    bool status();
  private:
    byte _pin_r;
    byte _pin_led;
    bool _status = false;
};

#endif
