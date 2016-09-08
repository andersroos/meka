#pragma once

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
                      CHANGE);
      reset();
   }

   // Set the current raw angle value and lap value to 0 and thus make it reference.
   void reset(int16_t raw=0, int32_t lap=0)
   {
      _raw = raw;
      _lap = lap;
   }

   // Return the raw angle value.
   ang_t raw()
   {
      return _raw;
   }

   // Return the lap value.
   int32_t lap()
   {
      return _lap;
   }
   
   // Return the relative angle between ang and ref (ang - ref). The return value will be in the range [-rev_tics/2,
   // rev_tics/2).
   inline ang_t rel(ang_t ang, ang_t ref)
   {
      ang_t half = rev_tics >> 1;
      return uint16_t(ang - ref + half) % rev_tics - half;
   }
   
   // Return the current angle compared to a raw reference point (ref). Returns a value in the range [-rev_tics/2,
   // rev_tics/2).
   inline ang_t ang(ang_t ref=0) 
   {
      return rel(_raw, ref);
   }

   virtual ~rotary_encoder() {}

//private:

   /*
     Vad kan vara fel?
     * Glappkontakt.
     * Fysisk glidning inne i encodern.
     * Fysisk glidning vid axeln.
     * Fel i algoritmen (missar steg vid vändning).
     * Missar interrupts pga tidsbrist.
     * Missar interrupts pga andra interrups (serial eller timer).
     * Dålig timing i digitalRead, kan bero på sena interrupts.

     Debuguppslag:
     * Gaffla Fysiskt/elekriskt fel vs logiskt/timingfel.
       - Kontrollräkna på nåt sätt med en dedikerad arduino/teensy? Gafflar ev timing, men inte logiskt fel.
       - Kontrollräkna med en hårdvaruräknare. Gafflar ev timing men, inte logiskt fel.
     * Tänk till om algoritm vid vändning, fungerar det verkligen.
     * Använd interrupts på b på nåt sätt för att kontrollera, kanske kan upptäcka elektriska fel, till exempel om man
       räknar på alla flanker.
     * Läs på hur andra gör.

     Ev lösning utan debuggning.
     * Byt till snabbare variant av digitalRead.
     * Lägg till den femte ledaren och justera vid varje varv.

     Observationer
     * Händer även när consolen är bortplockad.
     * Händer även när både huvudprintout och consolen är bortplockad.
     
   */
   
   static void interrupt()
   {
      bool a = digitalRead(encoder_a);
      bool b = digitalRead(encoder_b);
      if (a == b) {
         --_raw;
         if (_raw == -1) {
            _raw = rev_tics - 1;
            _lap--;
         }
      }
      else {
         ++_raw;
         if (_raw == rev_tics) {
            _raw = 0;
            _lap++;
         }
      }
   }

   static volatile ang_t   _raw;
   static volatile int32_t _lap;
};

template<pin_t encoder_a, pin_t encoder_b, uint16_t rev_tics>
volatile ang_t rotary_encoder<encoder_a, encoder_b, rev_tics>::_raw;

template<pin_t encoder_a, pin_t encoder_b, uint16_t rev_tics>
volatile int32_t rotary_encoder<encoder_a, encoder_b, rev_tics>::_lap;
