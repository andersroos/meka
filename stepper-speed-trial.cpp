//
// Speed test for stepper, set acceleration and speed.
//

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/stepper.hpp"

#define ACCELERATION 2000
#define SMOOTH_DELAY 300
#define SPEED        1000
#define DISTANCE     400
#define STEPS        1e9

#define DIR 2
#define STP 3
#define EN  7

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

void delay_unitl(uint32_t timestamp)
{
   while (timestamp >= now_us());
}

stepper stepper(DIR, STP, EN, M0, M1, M2, 1, SMOOTH_DELAY);

int32_t distance = DISTANCE;

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
   delay_unitl(stepper.off());
   
   digitalWrite(Y_LED, 1);
   digitalWrite(G_LED, 0);
   
   wait_for_button_relese(START_BUT);

   digitalWrite(Y_LED, 0);
   digitalWrite(G_LED, 1);
   
   stepper.target_speed(SPEED);
   stepper.acceleration(ACCELERATION);
   stepper.calibrate_position();
   delay_unitl(stepper.on());
   stepper.target_pos(DISTANCE);

   uint32_t min_time_step[6] = { 1e9, 1e9, 1e9, 1e9, 1e9, 1e9 };
   uint32_t max_time_step[6] = { 0 };
   uint32_t min_delay[6] = { 1e9, 1e9, 1e9, 1e9, 1e9, 1e9 };
   uint32_t max_delay[6] = { 0 };
   uint8_t min_micro = 255;
   uint8_t max_micro = 0;

   uint32_t max_stp = 0;
   uint32_t max_stp_pos = 0;
   uint32_t max_stp_micro = 0;
   
   for (uint32_t i = 0; i < STEPS; ++i) {
      uint32_t before = now_us();
      uint32_t timestamp = stepper.step();
      uint32_t duration = now_us() - before;
      uint8_t micro = stepper.micro();
      
      if (!timestamp) break;

      if (max_stp < duration) {
         max_stp = duration;
         max_stp_pos = stepper.pos();
         max_stp_micro = stepper.micro();
      }

      min_micro = min(micro, min_micro);
      max_micro = max(micro, max_micro);
      min_time_step[micro] = min(duration, min_time_step[micro]);
      max_time_step[micro] = max(duration, max_time_step[micro]);
      min_delay[micro] = min(timestamp - before, min_delay[micro]);
      max_delay[micro] = max(timestamp - before, max_delay[micro]);
         
      if (digitalRead(EMERGENCY_BUT)) {
         break;
      }

      delay_unitl(timestamp);
   }

   distance = -distance;
   
   Serial.println("not good");
   Serial.print(" min_micro ");
   Serial.print(min_micro);
   Serial.print(" max_micro ");
   Serial.print(max_micro);
   Serial.print(" max_stp_pos ");
   Serial.print(max_stp_pos);
   Serial.print(" max_stp_micro ");
   Serial.print(max_stp_micro);
   Serial.println();
   for (uint8_t i = 0; i < 6; ++i) {
      Serial.print(i);
      Serial.print(": min_stp ");
      Serial.print(min_time_step[i]);
      Serial.print(" max_stp ");
      Serial.print(max_time_step[i]);
      Serial.print(" min_delay ");
      Serial.print(min_delay[i]);
      Serial.print(" max_delay ");
      Serial.print(max_delay[i]);
      Serial.println();
   }
}
