//
// Inverted pendulum using the following major components:
//
// * Arduino Uno.
//
// * Pololu DRV8825 stepper motor driver.
//
// * NEMA17 bipolar stepper motor from Polou (no 2267, SOYO part no SY42STH38-1684A), 1.7 A,  2.8 V, 1.8 deg steps.
//
// * Continously rotating pot as a rotary encoder.
//

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/util.hpp"
#include "lib/stepper.hpp"
#include "lib/event.hpp"
#include "pendel-pins.hpp"


int state = 0;
void blink(event_queue& eq, const timestamp_t& when)
{
   digitalWrite(Y_LED, state++ & 1);
   eq.enqueue(blink, when + 200 * 1000);
}

void setup()
{
   event_queue event_queue;
   pinMode(Y_LED, OUTPUT);
   event_queue.enqueue(blink, now_us());
   event_queue.run();
}

void loop()
{
   delay(1000);
   digitalWrite(Y_LED, state++ & 1);
}
