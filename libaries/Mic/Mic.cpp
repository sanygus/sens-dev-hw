#include "Arduino.h"
#include "Mic.h"

Mic::Mic(int pin)
{
  _pin = pin;
}

void Mic::readNoise()
{
  int sum = 0;
  int countNotNull = 0;
  for (int i = 1; i < size_arr; i++) {
    _values[i - 1] = _values[i];
    if(i == (size_arr - 1)) {
      _values[i] = analogRead(_pin);
    }
    if (_values[i] > 0) {
      sum += _values[i];
      countNotNull++;
    }
  }
  if (countNotNull > 0) {
    _value = sum / countNotNull;
  } else {
    _value = 0;
  }
}

unsigned int Mic::getNoise()
{
  return _value;
}
