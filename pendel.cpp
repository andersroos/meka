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
#define APPROX_DISTANCE  1500

#define START_BUT     G_BUT
#define PAUS_BUT      Y_BUT
#define EMERGENCY_BUT R_BUT

event_queue eq;

stepper stepper(DIR, STP, EN, M0, M1, M2, DIR_O, SMOOTH_DELAY);

button start_but(START_BUT);
button paus_but(PAUS_BUT);
button emergency_but(EMERGENCY_BUT);

button m_end_switch(M_END, true);
button o_end_switch(O_END, true);

// Blinking led means active operation (running, pausing, emergency stopping), lit led means possible/expected input.
led y_led(Y_LED, OFF);
led g_led(G_LED, OFF);
led r_led(R_LED, OFF);

led_blinker y_led_blink(eq, y_led);
led_blinker g_led_blink(eq, g_led);
led_blinker r_led_blink(eq, r_led);

led builtin_led(BUILTIN_LED, OFF);

int32_t m_end_pos;
int32_t o_end_pos;

void check_for_emergency_stop(event_queue& eq, const timestamp_t& when);
void calibrate_standby(event_queue& eq, const timestamp_t& when);
void calibrate_move_clear_of_m_end(event_queue& eq, const timestamp_t& when);
void calibrate_find_m_end(event_queue& eq, const timestamp_t& when);
void calibrate_find_o_end(event_queue& eq, const timestamp_t& when);
void calibrate_calibrate(event_queue& eq, const timestamp_t& when);
void calibrate_center(event_queue& eq, const timestamp_t& when);

void finished(event_queue& eq, const timestamp_t& when);

void setup()
{
   Serial.begin(9600);
   pinMode(POT, INPUT);
}   

void loop()
{
   if (emergency_but.value()) {
      delay(100);
      return;
   }
   
   eq.reset();
   
   delay_unitl(stepper.off());

   g_led.on();
   y_led.off();
   r_led.on();
   builtin_led.off();

   eq.enqueue_now(check_for_emergency_stop);
   eq.enqueue_now(calibrate_standby);
   eq.run();
}

void calibrate_standby(event_queue& eq, const timestamp_t& when)
{
   if (not start_but.pressed()) {
      eq.enqueue(calibrate_standby, when + 100 * MILLIS);
      return;
   }

   g_led_blink.start(100 * MILLIS);

   // We need a fast acceleration to be able to stop when reaching end, but not crazy fast so we miss steps.
   stepper.acceleration(MAX_ACCELERATION * 0.7);
      
   // As fast as possible but we need to be able to stop before crashing.
   stepper.target_speed(1200);
      
   m_end_pos = 0;
   o_end_pos = 0;
   
   stepper.target_pos(0);
   stepper.calibrate_position(0);
 
   delay_unitl(stepper.on());

   if (m_end_switch.value()) {
      stepper.target_rel_pos(APPROX_DISTANCE * 0.2);
   }

   eq.enqueue_now(calibrate_move_clear_of_m_end);
}

void calibrate_move_clear_of_m_end(event_queue& eq, const timestamp_t& when)
{
   if (not stepper.is_stopped()) {
      eq.enqueue(calibrate_move_clear_of_m_end, stepper.step());
      return;
   }

   stepper.target_rel_pos(-APPROX_DISTANCE * 1.5);
   
   eq.enqueue_now(calibrate_find_m_end);
}

void calibrate_find_m_end(event_queue& eq, const timestamp_t& when)
{
   if (not m_end_switch.value() and not stepper.is_stopped()) {
      eq.enqueue(calibrate_find_m_end, stepper.step());
      return;
   }

   m_end_pos = stepper.pos();
   stepper.target_rel_pos(APPROX_DISTANCE * 1.5);
   eq.enqueue_now(calibrate_find_o_end);
}

void calibrate_find_o_end(event_queue& eq, const timestamp_t& when)
{
   if (not o_end_switch.value() and not stepper.is_stopped()) {
      eq.enqueue(calibrate_find_o_end, stepper.step());
      return;
   }
   
   o_end_pos = stepper.pos();
   stepper.target_pos(o_end_pos);
   eq.enqueue_now(calibrate_calibrate);
}

void calibrate_calibrate(event_queue& eq, const timestamp_t& when)
{
   if (not stepper.is_stopped()) {
      eq.enqueue(calibrate_calibrate, stepper.step());
      return;
   }

   o_end_pos = o_end_pos - m_end_pos;
   m_end_pos = 0;
   stepper.calibrate_position(o_end_pos);
   stepper.target_pos(o_end_pos / 2);
   eq.enqueue_now(calibrate_center);
}

void calibrate_center(event_queue& eq, const timestamp_t& when)
{
   if (not stepper.is_stopped()) {
      eq.enqueue(calibrate_center, stepper.step());
      return;
   }
   eq.enqueue_now(finished);
}

void finished(event_queue& eq, const timestamp_t& when)
{
   g_led_blink.stop();
   g_led.on();
   delay_unitl(stepper.off());
}

// void start(event_queue& eq, const timestamp_t& when)
// {
//    stepper.target_speed(MAX_SPEED);
//    stepper.acceleration(MAX_ACCELERATION);
//    eq.stop();
// }

void check_for_emergency_stop(event_queue& eq, const timestamp_t& when)
{
   if (emergency_but.value()) {
      builtin_led.on();   
      eq.stop();
      delay_unitl(stepper.off());
      return;
   }
   eq.enqueue(check_for_emergency_stop, when + 100 * MILLIS);
}

