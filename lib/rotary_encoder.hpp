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

   // Return the relative angle between ang and ref (ang - ref). The return value will be in the range [-rev_tics/2,
   // rev_tics/2).
   inline ang_t rel(ang_t ang, ang_t ref)
   {
      ang_t half = rev_tics >> 1;
      return (ang -  ref + half) % rev_tics - half;
   }
   
   // Return the current angle compared to a raw reference point (ref). Returns a value in the range [-rev_tics/2,
   // rev_tics/2).
   inline ang_t ang(ang_t ref=0) 
   {
      return rel(_raw, ref);
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

