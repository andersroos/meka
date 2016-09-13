//
// The Arduino Uno blink program, or just some other short test.
//

#include "Arduino.h"

#define BUILTIN_LED_PIN 13
#define Y_LED_PIN 14
#define G_LED_PIN 15

void setup() {
   pinMode(BUILTIN_LED_PIN, OUTPUT);
   pinMode(Y_LED_PIN, OUTPUT);
   pinMode(G_LED_PIN, OUTPUT);
   Serial.begin(9600);
}

int b = 0;
int g = 1;
int y = 0;

void loop() {
   digitalWrite(Y_LED_PIN, b++ & 1);
   digitalWrite(Y_LED_PIN, y++ & 1);
   digitalWrite(G_LED_PIN, g++ & 1);
   delay(100);
}
