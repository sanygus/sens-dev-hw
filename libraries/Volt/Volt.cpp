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
  for (int i = 1; i < countValues; i++) {
    _values[i - 1] = _values[i];
    if (_values[i] > 0) {
      sum += _values[i];
      countNotNull++;
    }
    if (i == (countValues - 1)) {
      _values[i] = analogRead(_pin);
      if (_values[i] > 0) {
        sum += _values[i];
        countNotNull++;
      }
    }
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
