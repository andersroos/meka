//
// Speed test for stepper, set acceleration and speed.
//

// POT
// 736 är ned
// 230 är upp
// ~470 är höger
// > 900 är opålitligt
// död zon mäter ca 860 (vilket är samma som snett neråt vänster, alltså bra grej)
// < 50 är opålitligt

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/util.hpp"
#include "lib/stepper.hpp"
#include "pins.hpp"

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

int32_t m_end_pos;
int32_t o_end_pos;

// Wait for M_BUT, return accel value based on M_POT, will print changes.
void setup_standby(uint32_t& accel)
{
   delay_unitl(stepper.off());

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
      Serial.println(analogRead(POT));
   }
}

// After waiting on button press, move stepper to a place where none of the limiter switches are enabled. Then move to
// the motor until the motor switch, reset the stepper, then move to the other side. Move motor to center pos and return
// other pos.
void calibrate(int32_t& m_end_pos, int32_t& o_end_pos)
{
   delay_unitl(stepper.off());

   // We need a fast acceleration to be able to stop when reaching end, but not crazy fast so we miss steps.
   stepper.acceleration(MAX_ACCELERATION * 0.7);

   // As fast as possible but we need to be able to stop before crashing.
   stepper.target_speed(1200);

   pin_value_t blink = 0;

   m_end_pos = 0;
   o_end_pos = 0;
   
   while (not start_but.pressed()) {
      digitalWrite(Y_LED, blink);
      blink = not blink;
      delay(200);
   }
   
   digitalWrite(Y_LED, 0);
   digitalWrite(G_LED, 1);

   delay_unitl(stepper.on());
   stepper.target_pos(0);
   stepper.calibrate_position();
   
   // If pos is at m end, move a bit so it is no longer lit.
   if (not digitalRead(M_END)) {

      stepper.target_rel_pos(200);
      
      while (true) {
         timestamp_t timestamp = stepper.step();
         if (not timestamp) break;
         delay_unitl(timestamp);

         if (emergency_but.value()) {
            stepper.off();
            return;
         }
      }
   }

   // Move until m end.
   stepper.target_rel_pos(-2000);
   bool detected = false;
   while (true) {
      timestamp_t timestamp = stepper.step();
      
      if (not detected and not digitalRead(M_END)) {
         detected = true;
         stepper.target_rel_pos(1);
      }
      
      if (not timestamp) break;
      delay_unitl(timestamp);
      
      if (emergency_but.value()) {
         stepper.off();
         return;
      }
   }
   stepper.calibrate_position();

   // Move stepper until o end.
   stepper.target_pos(2000);
   detected = false;
   while (true) {
      timestamp_t timestamp = stepper.step();
      
      if (not detected and not digitalRead(O_END)) {
         detected = true;
         stepper.target_rel_pos(-1);
      }
      
      if (not timestamp) break;
      delay_unitl(timestamp);

      if (emergency_but.value()) {
         stepper.off();
         return;
      }
   }
   o_end_pos = stepper.pos();

   // Move stepper to mid.
   stepper.target_pos(620);
   while (true) {
         timestamp_t timestamp = stepper.step();
         if (not timestamp) break;
         delay_unitl(timestamp);
   }

   Serial.print("o end ");
   Serial.println(o_end_pos);
   
   digitalWrite(Y_LED, 1);
   digitalWrite(G_LED, 0);
}

void setup()
{
   Serial.begin(9600);
   
   pinMode(START_BUT, INPUT);
   pinMode(EMERGENCY_BUT, INPUT);
   
   pinMode(Y_LED, OUTPUT);
   pinMode(G_LED, OUTPUT);
   pinMode(BUILTIN_LED, OUTPUT);

   pinMode(M_END, INPUT);
   pinMode(O_END, INPUT);
   
   pinMode(M_POT, INPUT);
   pinMode(O_POT, INPUT);

   pinMode(POT, INPUT);
   
   digitalWrite(BUILTIN_LED, 1);

   delay_unitl(stepper.off());

   calibrate(m_end_pos, o_end_pos);
}

void loop()
{
   Serial.println("standby");
   uint32_t accel = 0;

   setup_standby(accel);

   if (emergency_but.value()) {
      calibrate(m_end_pos, o_end_pos);
   }
   else {
      uint32_t target_pos = 0;
      timestamp_t pos_read = 0;
   
      stepper.target_speed(MAX_SPEED);
      stepper.acceleration(accel);

      digitalWrite(Y_LED, 0);
      digitalWrite(G_LED, 1);
   
      delay_unitl(stepper.on());
      
      Serial.println("running");
      
      do {
         uint32_t timestamp = stepper.step();
         
         if (now_us() - pos_read > 10000) {
            target_pos = map(analogRead(M_POT), 0, 1023, m_end_pos + 10, o_end_pos - 10);
            stepper.target_pos(target_pos);
            pos_read = now_us();
         }
      
         delay_unitl(timestamp);

         if (not digitalRead(M_END)) break;

         if (not digitalRead(O_END)) break;
         
      } while (not emergency_but.value());

      Serial.print("target pos ");
      Serial.println(stepper.target_pos());
      Serial.print("pos ");
      Serial.println(stepper.pos());
      
      delay_unitl(stepper.off());
   }
}
