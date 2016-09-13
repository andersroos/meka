//
// Speed test for stepper, set acceleration and speed.
//

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/stepper.hpp"

#define STP 3
#define DIR 2
#define EN  7

#define M0 6
#define M1 5
#define M2 4

#define SMOOTH_DELAY 200
#define ACCELERATION 10
#define SPEED        2000
#define DISTANCE     1500

#define DIR_0 0

#define BUILTIN_LED 13

stepper stepper(DIR, STP, EN, M0, M1, M2, DIR_0, SMOOTH_DELAY);

void delay_unitl(uint32_t timestamp)
{
   while (timestamp >= now_us());
}

pin_t led = 0;

void setup()
{
   Serial.begin(9600);
   
   pinMode(BUILTIN_LED, OUTPUT);
   
   stepper.target_speed(SPEED);
   stepper.acceleration(ACCELERATION);
}

void loop()
{
   stepper.calibrate_position();

   delay_unitl(stepper.on());
   stepper.target_pos(DISTANCE);

   while (stepper.pos() != stepper.target_pos()) {
      digitalWrite(BUILTIN_LED, led++ & 1);
      delay_unitl(stepper.step());
   }
   delay_unitl(stepper.off());
}
