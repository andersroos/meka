#include "Arduino.h"

int charge_pin = 4;
int discharge_pin = 8;
int measure_pin = A0;

void setup() {
   pinMode(discharge_pin, INPUT);
   pinMode(charge_pin, OUTPUT);
   digitalWrite(charge_pin, LOW);
   Serial.begin(9600);
}

void loop() {
   unsigned long start_time = micros();
   digitalWrite(charge_pin, HIGH);
   while (true) {
      int charge = analogRead(measure_pin);
      if (charge > 1023 * 0.6) {
         break;
      }
   }
   unsigned long duration = micros() - start_time;

   Serial.print("charge to 60%: ");
   Serial.println(duration);

   digitalWrite(charge_pin, LOW);
   pinMode(discharge_pin, OUTPUT);
   digitalWrite(discharge_pin, LOW);
   while (analogRead(measure_pin) > 0) {
      delay(1);
   }
   delay(1000);
   pinMode(discharge_pin, INPUT);
}
