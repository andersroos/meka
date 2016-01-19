//
// Speed test for stepper, set acceleration and speed.
//

#include "Arduino.h"

#define ACCELERATION 1000
#define SMOOTH_DELAY 700
#define SPEED        1000
#define DISTANCE     10000
#define STEPS        1e9

#define DIR 2
#define STP 3
#define EN  4

#define M0 x
#define M1 x
#define M2 x

stepper stepper(DIR, STP, EN, M0, M1, M2, 1, SMOOTH_DELAY);

void setup()
{
   stepper.target_speed(SPEED);
   stepper.acceleration(ACCELERATION);
   stepper.on();
   stepper.target_pos(DISTANCE);

   for (uint32_t i = 0; i < STEPS; ++i) {
      uint32_t timestamp = stepper.step();
      if (!timestamp) break;

      while (timestamp < micros());
   }

   stepper.off();
}

void loop()
{
}
