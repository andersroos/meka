//
// Speed test for stepper, set acceleration and speed.
//

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/util.hpp"
#include "lib/stepper.hpp"
#include "pendel-pins.hpp"

#define SMOOTH_DELAY 200
#define ACCELERATION 50000
#define MAX_SPEED    12000
#define DISTANCE     1e6
#define STEPS        1e9

#define BIG_INT uint32_t(1e9)

#define START_BUT     M_BUT
#define EMERGENCY_BUT O_BUT

void delay_unitl(uint32_t timestamp)
{
   while (timestamp >= now_us());
}

stepper stepper(DIR, STP, EN, M0, M1, M2, DIR_O, SMOOTH_DELAY);

button start_but(START_BUT);

button emergency_but(EMERGENCY_BUT);

int32_t distance = DISTANCE;
uint32_t speed = 0;

void setup()
{
   Serial.begin(9600);
   
   pinMode(START_BUT, INPUT);
   pinMode(EMERGENCY_BUT, INPUT);
   
   pinMode(Y_LED, OUTPUT);
   pinMode(G_LED, OUTPUT);
   pinMode(BUILTIN_LED, OUTPUT);

   digitalWrite(BUILTIN_LED, 1);

   pinMode(M_POT, INPUT);
}

void loop()
{
   delay_unitl(stepper.off());
   
   digitalWrite(Y_LED, 1);
   digitalWrite(G_LED, 0);
   
   while (not start_but.pressed()) {
      speed = analogRead(M_POT);
      Serial.println(speed);
      delay(50);
   }

   digitalWrite(Y_LED, 0);
   digitalWrite(G_LED, 1);

   stepper.target_speed(min(speed, uint32_t(MAX_SPEED)));
   stepper.acceleration(ACCELERATION);
   stepper.calibrate_position();
   delay_unitl(stepper.on());
   stepper.target_pos(distance);

   for (uint32_t i = 0; i < STEPS; ++i) {
      uint32_t timestamp = stepper.step();

      speed = analogRead(M_POT);
      stepper.target_speed(min(speed, uint32_t(MAX_SPEED)));

      if (!timestamp) break;

      if (emergency_but.value()) {
         break;
      }

      delay_unitl(timestamp);
   }

   distance = -distance;
}
