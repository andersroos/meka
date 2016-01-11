//
// Basic testing of controlling a stepper motor with Arduino Uno.
//

#include "Arduino.h"

#define STP 3
#define DIR 2
#define EN  7

#define M0 6
#define M1 5
#define M2 4

void setup()
{
   pinMode(STP, OUTPUT);
   pinMode(DIR, OUTPUT);
   pinMode(EN,  OUTPUT);

   pinMode(M0, OUTPUT);
   pinMode(M1, OUTPUT);
   pinMode(M2, OUTPUT);

   digitalWrite(M0, 0);
   digitalWrite(M1, 0);
   digitalWrite(M2, 0);

   digitalWrite(STP, 0);
   digitalWrite(DIR, 0);

   digitalWrite(EN, 1);

   pinMode(13, OUTPUT);
   digitalWrite(13, 0);
   
   Serial.begin(9600);

   delay(1000);

   digitalWrite(13, 1);
   digitalWrite(EN, 0);   
}

int step = 0;

void loop()
{
   digitalWrite(STP, step++ & 1);
   delayMicroseconds(350);
   
   // delayMicroseconds(350); // Fastest possible for Paulu NEMA 17.
   // delayMicroseconds(800); // Fastet possible for Mercury Motor NEMA 17.
   // delayMicroseconds(650); // Fastes possible for Wantai NEMA 23 (4 leads, funkar sådär).
}
