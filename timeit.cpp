//
// Timing of calculations on a Arduino Uno, division is pretty slow, which is no surpise.
//

#include "Arduino.h"

uint32_t n = 123489853;
uint32_t t = 1343542;

void setup()
{ 
   Serial.begin(9600);
}

uint32_t t0;
uint32_t t1;
uint16_t sum = 0;
uint32_t count;

#define START 9000

void loop()
{
   count = 0;
   sum = 0;

   // float division => 48 us => 768 cycles
   // float multiplication => 27 us => 432 cycles
   // float sum => 24 us => 384 cycles
   // uint32 multiplication => ~1 us
   // uint32 sum => ~1 us
   // uint32 division => 38 us => 608 cycles
   // uint32 multiplication => ~1 us
   // uint32 sum => ~1 us
   // uint32 count in loop, aka measurment error => 0.001 us (which is a lie because less than clock cycle)
   // uint16 division => 13 us => 208 cycles
   // uint16 multiplication => ~1 us
   // uint8  division => ~5 us => 80 cycles
   // uint8  multiplication => ~1 us
   
   
   t0 = micros();
   for (uint16_t x = START + 1; x < START + 1e3; ++x) {
      for (uint16_t y = START + 1; y < START + 1e3; ++y) {
         sum += x % y + x / y;
         count++;
      }
   }
   t1 = micros();
   
   Serial.print("time: ");
   Serial.print(t1 - t0);
   Serial.print(" count: ");
   Serial.print(count);
   Serial.print(" avg: ");
   Serial.print((t1 - t0) / count);
   Serial.print(" sum: ");
   Serial.println(sum);
}
