//
// Controlling an ESC using servo controller code on an Arduino Uno.
//

#include "Arduino.h"
#include "Servo.h"

#define ESC 6
#define LED 13

Servo servo;

int speed = 1000;

void setup()
{
   pinMode(LED, OUTPUT);
   Serial.begin(9600);
   Serial.println("Sending start sequence to ESC (wait).");

   servo.attach(6);
   servo.writeMicroseconds(1000);
   digitalWrite(LED, 1);
   delay(3000);
   
   servo.writeMicroseconds(1200);
   
   digitalWrite(LED, 0);
   delay(1000);
   digitalWrite(LED, 1);
   
   servo.writeMicroseconds(1000);
   Serial.println("Init done.");
}

void loop()
{

   int r = Serial.read();
   if (r == 65) {
      speed -= 50;
      Serial.print("Setting speed to  ");
      Serial.println(speed);
      servo.writeMicroseconds(speed);
   }
   if (r == 66) {
      speed += 50;
      Serial.print("Setting speed to  ");
      Serial.println(speed);
      servo.writeMicroseconds(speed);
   }
}
