//
// Testing a servo using an Arduino Uno.
//

#include "Arduino.h"
#include "Servo.h"
#include "lib/base.hpp"
#include "lib/util.hpp"
#include "lib/event_queue.hpp"
#include "lib/event_utils.hpp"
#include "lib/serial.hpp"

// Three buttons, up and down duty cycle and one toggle pwm.
constexpr pin_t DOWN = 2;
constexpr pin_t UP = 3;
constexpr pin_t POWER = 4;

constexpr pin_t SERVO = 9;

constexpr pin_t LED = 13;

Servo servo;
 
led i_led(LED);
button up_but(UP);
button down_but(DOWN);
button power_but(POWER);
 
event_queue eq;
 
led_blinker led_blink(eq, i_led);
 
noblock_serial serial(&eq);

bool on = false;
int16_t ang = 0;
int16_t last_ang = 0;

void doit(event_queue& eq, const timestamp_t& when);   

void setup()
{
   i_led.off();
   //serial.begin();
   //serial.p("setup\n");
   
   eq.enqueue_now(doit);
   eq.run();

}

void loop() {
}


void doit(event_queue& eq, const timestamp_t& when)
{
    bool up_pressed = up_but.value();
    bool down_pressed = down_but.value();
    bool power_pressed = power_but.pressed();
 
    if (up_pressed) {
       ang = min(180, ang + 9);
    }
    else if (down_pressed) {
       ang = max(0, ang - 9);
    }
    else if (power_pressed) {
       if (on) {
          servo.detach();
          led_blink.stop();
          //serial.p("stopping\n");
       }
       else {
          servo.attach(SERVO);
          led_blink.start(100 * MILLIS);
          last_ang = -1;
          //serial.p("starting\n");
       }
       on = not on;
    }
 
    if (on and ang != last_ang) {
       led_blink.interval((100 + ang * 4) * MILLIS);
       servo.write(ang);
       last_ang = ang;
       //serial.p(ang, "\n");
    }

    eq.enqueue_rel(doit, 10 * MILLIS);
}

