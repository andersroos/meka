//
// Speed test for stepper, set acceleration and speed.
//

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/stepper.hpp"

#define ACCELERATION 2e4
#define SMOOTH_DELAY 700
#define SPEED        10000
#define DISTANCE     1500
#define STEPS        1e9

#define DIR 2
#define STP 3
#define EN  4

#define M0 6
#define M1 5
#define M2 4

#define START_BUT 12
#define EMERGENCY_BUT 13

#define Y_LED 10
#define G_LED 11

void wait_for_button_relese(uint8_t but)
{
   uint8_t last = 0;
   uint8_t val = 0;
   do {
      last = val;
      val = digitalRead(but);
      delay(10); // delay will hopefully make button settle itself a bit before next read
   } while (last <= val);
}

stepper stepper(DIR, STP, EN, M0, M1, M2, 1, SMOOTH_DELAY);

void setup()
{
   Serial.begin(9600);
   
   pinMode(START_BUT, INPUT);
   pinMode(EMERGENCY_BUT, INPUT);
   
   pinMode(Y_LED, OUTPUT);
   pinMode(G_LED, OUTPUT);
}

void loop()
{
   wait_for_button_relese(START_BUT);

   digitalWrite(Y_LED, 0);
   digitalWrite(G_LED, 1);
   
   stepper.target_speed(SPEED);
   stepper.acceleration(ACCELERATION);
   stepper.calibrate_position();
   stepper.on();
   stepper.target_pos(DISTANCE);
   
   for (uint32_t i = 0; i < STEPS; ++i) {
      // uint32_t start = now_us();
      uint32_t timestamp = stepper.step();
      // Serial.print(i);
      // Serial.print(" ");
      // Serial.print(timestamp - start);
      // Serial.print(" ");
      // Serial.print(stepper.pos());
      // Serial.print(" ");
      // Serial.print(stepper.micro());
      // Serial.println(" ");
      
      if (!timestamp) break;
      
      if (digitalRead(EMERGENCY_BUT)) {
         break;
      }

      while (timestamp >= now_us());
   }
   
   digitalWrite(Y_LED, 1);
   digitalWrite(G_LED, 0);
   
   stepper.off();
}
