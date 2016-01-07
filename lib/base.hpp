//
// Lib for cpp micro controller base stuff.
// 
//

#ifndef MECHATRONICS_EVENT_HPP
#define MECHATRONICS_EVENT_HPP

// Led pin to flash in case of errors.
#ifndef LED_PIN
#define LED_PIN 14
#endif

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
      
#endif
