#include "Arduino.h"
#include "Volt.h"

Volt::Volt(int pin)
{
  _pin = pin;
}

void Volt::readVolt()
{
  unsigned int sum = 0;
  int countNotNull = 0;
  for (int i = 0; i < count; i++) {
    _tmp = analogRead(_pin);
    if (_tmp > 0) {
      sum += _tmp;
      countNotNull++;
    }
    delay(5);
  }
  if (countNotNull > 0) {
    _value = sum / countNotNull;
  } else {
    _value = 0;
  }
}

unsigned int Volt::getVolt()
{
  return _value;
}
