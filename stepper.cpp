#include "Arduino.h"

#define STP 2
#define DIR 3
#define MS1 4
#define MS2 5
#define MS3 6
#define EN  7
#define B   10

bool run = false;
int b = 0;

int step = 0;

void setup()
{
   pinMode(STP, OUTPUT);
   pinMode(DIR, OUTPUT);
   pinMode(MS1, OUTPUT);
   pinMode(MS2, OUTPUT);
   pinMode(MS3, OUTPUT);
   pinMode(EN,  OUTPUT);
   pinMode(B,   INPUT);

   digitalWrite(STP, 0);
   digitalWrite(DIR, 0);
   
   digitalWrite(MS1, 0);
   digitalWrite(MS2, 0);
   digitalWrite(MS3, 0);

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

   // Around 800 is max.

   delay(1000);
   //delayMicroseconds(80000);
}
