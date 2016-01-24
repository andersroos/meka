//
// The Arduino Uno blink program, or just some other short test.
//

#include "Arduino.h"

#define Y_LED_PIN 14
#define G_LED_PIN 15

void setup() {
   pinMode(Y_LED_PIN, OUTPUT);
   pinMode(G_LED_PIN, OUTPUT);
   Serial.begin(9600);
}

void loop() {
   Serial.println("Hejhej");
   
   digitalWrite(Y_LED_PIN, 1);
   digitalWrite(G_LED_PIN, 0);
   delay(100);
   digitalWrite(Y_LED_PIN, 0);
   digitalWrite(G_LED_PIN, 1);
   delay(100);
   digitalWrite(Y_LED_PIN, 1);
   digitalWrite(G_LED_PIN, 0);
   delay(100);
   digitalWrite(Y_LED_PIN, 0);
   digitalWrite(G_LED_PIN, 1);
   delay(100);
   digitalWrite(Y_LED_PIN, 0);
   digitalWrite(G_LED_PIN, 0);
   delay(500);
}
