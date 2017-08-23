#ifndef Volt_h
#define Volt_h
#define countValues 10

#include "Arduino.h"

class Volt
{
  public:
    Volt(int pin);
    void readVolt();
    unsigned int getVolt();
    float getCharge(unsigned int min, unsigned int max);
  private:
    byte _pin;
    unsigned int _values[countValues];
    unsigned int _value;
};

#endif
