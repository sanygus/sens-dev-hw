#ifndef Volt_h
#define Volt_h
//#define size_arr 10
#define count 10

#include "Arduino.h"

class Volt
{
  public:
    Volt(int pin);
    void readVolt();
    unsigned int getVolt();
  private:
    byte _pin;
    //unsigned int _values[size_arr];
    unsigned int _tmp;
    unsigned int _value;
};

#endif
