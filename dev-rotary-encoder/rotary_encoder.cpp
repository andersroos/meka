//
// Testing the 1024 pulse/lap quadratic rotary encoder ob a Arduino Uno (easier since it uses 5v).
//

#include "Arduino.h"

#define A 2
#define Z 3
#define B 4

volatile int ang = 0;
volatile bool lap = false;

void tick()
{
   byte b = digitalRead(B);
   if (b) {
      ++ang;
   }
   else {
      --ang;
   }
}

void tock()
{
   lap = true;
}

void setup()
{ 
   Serial.begin(9600);
   pinMode(A, INPUT);
   pinMode(B, INPUT);

   attachInterrupt(digitalPinToInterrupt(A), tick, RISING);
   attachInterrupt(digitalPinToInterrupt(Z), tock, RISING);
}
   
void loop()
{
   Serial.print(ang);
   Serial.print("\n");

   if (lap) {
      lap = false;
      Serial.println("lap");
   }
   
   delay(100);
}
