//
// Speed test for stepper, set acceleration and speed.
//

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/util.hpp"
#include "lib/stepper.hpp"
#include "pendel-pins.hpp"

#define SMOOTH_DELAY       200
#define MAX_ACCELERATION 50000
#define MAX_SPEED        12000

#define START_BUT     M_BUT
#define EMERGENCY_BUT O_BUT

void delay_unitl(uint32_t timestamp)
{
   while (timestamp >= now_us());
}

stepper stepper(DIR, STP, EN, M0, M1, M2, DIR_O, SMOOTH_DELAY);

button start_but(START_BUT);

button emergency_but(EMERGENCY_BUT);

void setup()
{
   Serial.begin(9600);
   
   pinMode(START_BUT, INPUT);
   pinMode(EMERGENCY_BUT, INPUT);
   
   pinMode(Y_LED, OUTPUT);
   pinMode(G_LED, OUTPUT);
   pinMode(BUILTIN_LED, OUTPUT);

   pinMode(M_POT, INPUT);
   pinMode(O_POT, INPUT);

   digitalWrite(BUILTIN_LED, 1);

   delay_unitl(stepper.off());
   
   stepper.target_speed(MAX_SPEED);
   stepper.calibrate_position(); 
}

uint32_t target_pos = 0;
timestamp_t pos_read = 0;

uint32_t accel = 0;


void loop()
{
   
   digitalWrite(Y_LED, 1);
   digitalWrite(G_LED, 0);

   uint32_t val = 0;
   uint32_t last_val = 0;
   while (not start_but.pressed()) {
      val = analogRead(O_POT);
      accel = map(val, 0, 1023, 0, MAX_ACCELERATION);
      if (abs(val - last_val) > 2) {
         Serial.print("acceleration ");
         Serial.println(accel);
         last_val = val;
         delay(50);
      }
   }
   stepper.acceleration(accel);

   digitalWrite(Y_LED, 0);
   digitalWrite(G_LED, 1);
   
   delay_unitl(stepper.on());

   Serial.println("running");
      
   do {
      uint32_t timestamp = stepper.step();

      if (now_us() - pos_read > 10000) {
         target_pos = map(analogRead(M_POT), 0, 1023, 0, 1500);
         stepper.target_pos(target_pos);
         pos_read = now_us();
      }
      
      delay_unitl(timestamp);
      
   } while (not emergency_but.value());

   Serial.println("standby");

   delay_unitl(stepper.off());

}
