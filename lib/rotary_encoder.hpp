#pragma once

int x = 0;

//
// Interface for rotary, encoder_a which should be an interrupt port, and encoder_b which does not need to be, and then
// ticks per revolution.
//
template<pin_t encoder_a, pin_t encoder_b, uint16_t rev_tics=1024>
struct rotary_encoder
{
   // Init the encoder.
   rotary_encoder()
   {
      pinMode(13, OUTPUT);
      pinMode(encoder_a, INPUT);
      pinMode(encoder_b, INPUT);
      attachInterrupt(digitalPinToInterrupt(encoder_a), 
                      interrupt, 
                      RISING);
      reset();
   }

   // Set the current raw count to 0 and thus make it reference.
   void reset()
   {
      _raw = 0;
   }

   // Return the raw counter value.
   uint16_t raw()
   {
      return _raw;
   }

   // Return the current angle compared to a raw reference point (ref). Returns a value in the range [-rev_tics/2,
   // rev_tics/2).
   int16_t ang(int16_t ref=0) 
   {
      int16_t half = rev_tics >> 1;
      return (_raw - ref + half) % rev_tics - half;
   }

   // Return the relative angle compared to ang both according to a raw reference point (ref). The return value will be
   // in the range [-rev_tics/2, rev_tics/2).
   int16_t rel(int16_t ang, int16_t ref=0)
   {
      return this->ang(ref + ang);
   }
   
   virtual ~rotary_encoder() {}

private:
   
   static void interrupt()
   {
      byte b = digitalRead(encoder_b);
      if (b) {
         ++_raw;
      }
      else {
         --_raw;
      }
   }

   static volatile uint16_t _raw;
};

template<pin_t encoder_a, pin_t encoder_b, uint16_t pulses_per_lap>
volatile uint16_t rotary_encoder<encoder_a, encoder_b, pulses_per_lap>::_raw;

