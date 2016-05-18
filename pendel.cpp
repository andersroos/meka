//
// Inverted pendulum using the following major components:
//
// * Arduino Uno.
//
// * Pololu DRV8825 stepper motor driver.
//
// * NEMA17 bipolar stepper motor from Polou (no 2267, SOYO part no SY42STH38-1684A), 1.7 A,  2.8 V, 1.8 deg steps.
//
// * Continously rotating pot as a rotary encoder.
//

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/util.hpp"
#include "lib/stepper.hpp"
#include "lib/event_queue.hpp"
#include "pendel_pins.hpp"

#define SMOOTH_DELAY       200
#define MAX_ACCELERATION 50000
#define MAX_SPEED        12000

#define START_BUT     M_BUT
#define EMERGENCY_BUT O_BUT

event_queue eq;

stepper stepper(DIR, STP, EN, M0, M1, M2, DIR_O, SMOOTH_DELAY);

button start_but(START_BUT);
//button_waiter start_but_wait(start_but);

button emergency_but(EMERGENCY_BUT);

led y_led(Y_LED, OFF);
led g_led(G_LED, OFF);

led_blinker g_led_blink(eq, y_led);
led_blinker y_led_blink(eq, g_led);

led builtin_led(BUILTIN_LED, OFF);

int32_t m_end_pos;
int32_t o_end_pos;

// Run the stepper thowards its target position.
// void step(event_queue& eq, const timestamp_t& when)
// {
//    
// }

void init()
{
   Serial.begin(9600);

   pinMode(START_BUT, INPUT);
   pinMode(EMERGENCY_BUT, INPUT);
   
   pinMode(BUILTIN_LED, OUTPUT);
   
   pinMode(M_END, INPUT);
   pinMode(O_END, INPUT);
   
   pinMode(M_POT, INPUT);
   pinMode(O_POT, INPUT);

   pinMode(POT, INPUT);

   //delay_unitl(stepper.off());
   
   builtin_led.on();
}   

void start(event_queue& eq, const timestamp_t& when)
{
   y_led_blink.start(200 * MILLIS);

   // eq.enqueue_now(wait_for_)
   
   // while (not start_but.pressed());

   loop();
   y_led_blink.stop();
   
   //calibrate(m_end_pos, o_end_pos);

   stepper.target_speed(MAX_SPEED);
   stepper.acceleration(MAX_ACCELERATION);

   eq.stop();
}

void setup()
{
   init();
   eq.enqueue_now(start);
   eq.run();
}

void loop() {}

