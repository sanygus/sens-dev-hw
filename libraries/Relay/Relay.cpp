#include "Arduino.h"
#include "Relay.h"

Relay::Relay(int pin_r, int pin_led)
{
  _pin_r = pin_r;//пин реле
  _pin_led = pin_led;//пин светодиода
}

void Relay::on()
{
  digitalWrite(_pin_r, true);
  digitalWrite(_pin_led, false);
  _status = true;
}

void Relay::off()
{
  digitalWrite(_pin_r, false);
  digitalWrite(_pin_led, true);
  _status = false;
}

bool Relay::status()
{
  return _status;
}
