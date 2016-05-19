#pragma once

constexpr uint32_t MAX_TIMESTAMP = 0xffffffff; // 4.23e9 us or 71.58 minutes
constexpr uint32_t MILLIS = 1000;
constexpr uint32_t SECOND = 1000 * MILLIS;
constexpr uint32_t MINUTE = 60 * SECOND;

// Returns true if x is before y. Can only used reliably for values in the interval -5 mins to +65 mins from now.
bool before(const timestamp_t& now, const timestamp_t& x, const timestamp_t& y) {
   constexpr uint32_t _5_MINS = 5 * 60 * 1000 * 1000;
   return uint32_t(x - now + _5_MINS) < uint32_t(y - now + _5_MINS);
}

// Busy wait until timestamp, timestamp can be int he interval -5 mins to +65 minutes from now.
void delay_unitl(const timestamp_t& timestamp)
{
   while (true) {
      timestamp_t now = now_us();
      if (before(now, timestamp - 1, now)) {
         break;
      }
   }
}

// Util for reading button (switch).
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
   
   // Read button value and return true when a release is detected (low flank).
   bool pressed()
   {
      pin_value_t val = digitalRead(_pin);
      if (val < _lastval) {
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

// Util for single led.
struct led
{
   led(pin_t pin, pin_value_t val=0) : _pin(pin), _val(val)
   {
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, _val);
   }

   void on()
   {
      _val = 1;
      digitalWrite(_pin, _val);
   }

   void off()
   {
      _val = 0;
      digitalWrite(_pin, _val);
   }

   void toggle()
   {
      _val = not _val;
      digitalWrite(_pin, _val);
   }
   
private:
   pin_t       _pin;
   pin_value_t _val;
};

