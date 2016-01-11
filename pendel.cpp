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

#define M_POT A0
#define O_POT A1

#define M_BUT 12
#define O_BUT 13

#define Y_LED 10
#define G_LED 11

#define M_END 8
#define O_END 9

#define POT A2

#define DIR 2
#define STP 3
#define EN  4

#define DIR_M 1
#define DIR_O 0

#define ENABLE  0
#define DISABLE  1

// Resultat:
// speed på 10k steps/s och 30V brände drivern
// skulle tro att speed på 8k och kanske 18V är rätt bra

int speed;    // steps/s, +/-
int step = 0;

int distance = 800;   // target distance
int safe_speed = 800;

void setup()
{
   pinMode(Y_LED, OUTPUT);
   pinMode(G_LED, OUTPUT);

   digitalWrite(Y_LED, 0);
   digitalWrite(G_LED, 0);

   pinMode(M_BUT, INPUT);
   pinMode(O_BUT, INPUT);
   pinMode(M_END, INPUT);
   pinMode(O_END, INPUT);

   pinMode(DIR, OUTPUT);
   pinMode(STP, OUTPUT);
   pinMode(EN,  OUTPUT);

   digitalWrite(EN,  DISABLE);
   digitalWrite(DIR, DIR_M);
   digitalWrite(STP, step);
   
   Serial.begin(9600);
}

int wait(int speed)
{
   return 5e5 / speed;
}

int get_pot(int pot, char* name, int val, float factor)
{
   int now = analogRead(pot) * factor;
   delay(1);
   if (abs(now - val) > 2 * factor) {
      Serial.print(name);
      Serial.println(now);
      return now;
   }
   return val;
}

int m_pot = 0;
int o_pot = 0;
int target_speed = 1000;
int accel = 1000;

void loop()
{
   digitalWrite(EN, DISABLE);
   digitalWrite(Y_LED, 0);
   digitalWrite(G_LED, 1);

   // Setup parameters.
   
   while (true) {
      m_pot = get_pot(M_POT, "speed: ", m_pot, 12);
      o_pot = get_pot(O_POT, "accel: ", o_pot, 2);
      
      if (digitalRead(M_BUT)) {
         target_speed = m_pot;
         accel = o_pot;
         Serial.print("delay: ");
         Serial.print(1e6 / (2 * target_speed));
         Serial.print(", speed: ");
         Serial.print(target_speed);
         Serial.print(", accel: ");
         Serial.print(accel);
         Serial.println("");
         digitalWrite(Y_LED, 1);
         digitalWrite(G_LED, 0);
         break;
      }
   }

   // Move to motor side.

   digitalWrite(EN, ENABLE);
   digitalWrite(DIR, DIR_M);
   while (digitalRead(M_END)) {
      digitalWrite(STP, step++ & 1);
      delayMicroseconds(wait(safe_speed));
   }

   // Move distance steps.

   speed = 0;
   digitalWrite(DIR, DIR_O);
   int decel_steps = (target_speed - safe_speed) / accel;
   for (step = 0; step < distance * 2; ++step) {
      if (!digitalRead(O_END)) {
         break;
      }
      digitalWrite(STP, step & 1);
      speed += accel;
      speed = speed > target_speed ? target_speed : speed;

      int steps_left = distance * 2 - step;
      if (steps_left <= decel_steps) {
         delayMicroseconds(wait(safe_speed + steps_left * accel));
      }
      else {
         delayMicroseconds(wait(speed));
      }
   }

   // Count steps to motor side.

   digitalWrite(DIR, DIR_M);
   step = 0;
   while (digitalRead(M_END)) {
      digitalWrite(STP, step++ & 1);
      delayMicroseconds(wait(safe_speed));
   }

   Serial.print("distance: ");
   Serial.print(distance);
   Serial.print(" step: ");
   Serial.print(step / 2);
   Serial.print(", speed: ");
   Serial.print(target_speed);
   Serial.print(", accel: ");
   Serial.print(accel);
   Serial.println("");
   
   //    // delayMicroseconds(350); // Fastest possible for Paulu NEMA 17.
   //    // delayMicroseconds(800); // Fastet possible for Mercury Motor NEMA 17.
   //    // delayMicroseconds(650); // Fastes possible for Wantai NEMA 23 (4 leads, funkar sådär).
   // }
}
