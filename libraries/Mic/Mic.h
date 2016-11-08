#ifndef Mic_h
#define Mic_h
#define size_arr 30

#include "Arduino.h"

class Mic
{
  public:
    Mic(int pin);
    void readNoise();
    unsigned int getNoise();
  private:
    byte _pin;
    unsigned int _values[size_arr];
    unsigned int _value;
};

#endif
