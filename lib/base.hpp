#pragma once

//
// Lib for cpp micro controller base stuff.
//

// Led pin to flash in case of errors.
#ifndef LED_PIN
#define LED_PIN 14
#endif

using pin_t       = uint8_t;
using pin_value_t = uint8_t;

using delay_t     = uint32_t;
using timestamp_t = uint32_t;


// Blink error message, the blink will be 1s off then 125ms on/off for each bit in argument. So for example
// 0b11001100 will be two 250ms flashes, then 1250ms off.
void blink_error(uint8_t message)
{
   pinMode(LED_PIN, OUTPUT);
   while (true) {
      uint8_t msg = message;
      digitalWrite(LED_PIN, 0);
      delay(1000);
      for (uint8_t i = 0; i < 8; ++i) {
         digitalWrite(LED_PIN, msg & 0b10000000);
         msg = msg << 1;
         delay(125);
      }
   }
}

// Return current time in micro seconds since device power up.
timestamp_t now_us()
{
   return micros();
}
