#pragma once

// Util for reading button.
struct button
{

   // Init with pin.
   button(pin_t pin) : _pin(pin), _lastval(0)
   {
      pinMode(pin, INPUT);
   }

   // Read button value and return it.
   bool value()
   {
      return digitalRead(_pin);
   }
   
   // Read button value and return true a press and then a release was detected.
   bool pressed()
   {
      pin_value_t val = digitalRead(_pin);
      if (_lastval > val) {
         _lastval = 0;
         return true;
      }
      _lastval = val;
      return false;
   }

private:
   pin_t   _pin;
   pin_value_t _lastval;
};

