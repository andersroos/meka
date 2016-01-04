#include "Arduino.h"

#define STP 3
#define DIR 2
#define EN  4

#define B   12

bool run = false;
int b = 0;

int step = 0;

void setup()
{
   pinMode(STP, OUTPUT);
   pinMode(DIR, OUTPUT);
   pinMode(EN,  OUTPUT);
   
   pinMode(B,   INPUT);

   digitalWrite(STP, 0);
   digitalWrite(DIR, 0);
   digitalWrite(EN, 1);

   Serial.begin(9600);
}

void loop()
{
   int b_now = digitalRead(B);
   if (b > b_now) {
      run = !run;
      Serial.println(run ? "runing": "stopped");
      digitalWrite(EN, !run);
   }
   b = b_now;

   digitalWrite(STP, step++ & 1);

   delay(1000);
   
   // delayMicroseconds(350); // Fastest possible for Paulu NEMA 17.
   // delayMicroseconds(800); // Fastet possible for Mercury Motor NEMA 17.
   // delayMicroseconds(650); // Fastes possible for Wantai NEMA 23 (4 leads, funkar sådär).
}
