#include "Arduino.h"
#include "Volt.h"

Volt::Volt(int pin)
{
  _pin = pin;
}
//обновляет текущее значений вольтажа, пересчитывает среднее значение
void Volt::readVolt()
{
  unsigned int sum = 0;
  int countNotNull = 0;
  for (int i = 1; i < countValues; i++) {
    //сдвигаем текущие значения
    _values[i - 1] = _values[i];
    if (_values[i] > 0) {
      sum += _values[i];
      countNotNull++;
    }
    //обновляем последний элемент текущим значением
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

float Volt::getCharge(unsigned int min, unsigned int max)
{
  //перевод текущего значения АЦП в % заряда в диапазоне min max
  float charge = 0;
  if ((_value >= min) && (_value <= max)) {
    charge = ((float)(_value - min) / (float)(max - min));
  } else if ((_value > max)) {
    charge = 1;
  }
  return charge;
}
