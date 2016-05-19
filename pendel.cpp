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

#define START_BUT     M_BUT
#define EMERGENCY_BUT O_BUT

event_queue eq;

stepper stepper(DIR, STP, EN, M0, M1, M2, DIR_O, SMOOTH_DELAY);

button start_but(START_BUT);
button emergency_but(EMERGENCY_BUT);

button m_end_inv_switch(M_END);
button o_end_inv_switch(O_END);

led y_led(Y_LED, OFF);
led g_led(G_LED, OFF);

led_blinker y_led_blink(eq, y_led);
led_blinker g_led_blink(eq, g_led);

led builtin_led(BUILTIN_LED, OFF);

int32_t m_end_pos;
int32_t o_end_pos;

void check_for_emergency_stop(event_queue& eq, const timestamp_t& when);
void standby_for_calibrate(event_queue& eq, const timestamp_t& when);
void calibrate_finished(event_queue& eq, const timestamp_t& when);

void setup()
{
   Serial.begin(9600);

   pinMode(M_POT, INPUT);
   pinMode(O_POT, INPUT);

   pinMode(POT, INPUT);

   delay_unitl(stepper.off());
   
   y_led_blink.start(200 * MILLIS);

   eq.enqueue_now(check_for_emergency_stop);
   eq.enqueue_now(standby_for_calibrate);
   eq.run();
}   

void check_for_emergency_stop(event_queue& eq, const timestamp_t& when)
{
   if (emergency_but.value()) {
      builtin_led.on();   
      eq.stop();
      delay_unitl(stepper.off());
      stepper.off();
      y_led.off();
      g_led.off();
      return;
   }
   eq.enqueue(check_for_emergency_stop, when + 100 * MILLIS);
}

// After waiting on button press, move stepper to a place where none of the limiter switches are enabled. Then move to
// the motor until the motor switch, reset the stepper, then move to the other side. Move motor to center pos and return
// other pos.
struct calibrate : event_queue::callback_obj
{
   enum state : uint8_t { INIT, MID, FIND_M_END, FIND_O_END, CALIBRATE, CENTER };
      
   int32_t& m_end_pos;
   int32_t& o_end_pos;
   state state;

   calibrate(int32_t& m_end_pos, int32_t& o_end_pos) 
      : m_end_pos(m_end_pos), o_end_pos(o_end_pos), state(INIT)
   {}

   void operator()(event_queue& eq, const timestamp_t& when) {
         
      timestamp_t next_action = 0;
         
      switch (state) {
         case INIT: // Init everything for calibrating, called once.
               
            // We need a fast acceleration to be able to stop when reaching end, but not crazy fast so we miss steps.
            stepper.acceleration(MAX_ACCELERATION * 0.7);

            // As fast as possible but we need to be able to stop before crashing.
            stepper.target_speed(1200);

            m_end_pos = 0;
            o_end_pos = 0;
   
            delay_unitl(stepper.on());
            stepper.target_pos(0);
            stepper.calibrate_position();
               
            state = MID;
            if (not m_end_inv_switch.value()) {
               stepper.target_rel_pos(APPROX_DISTANCE * 0.2);
            }
            next_action = now_us();
            break;

         case MID: // Move stepper so m_end is no longer touched if needed.

            next_action = stepper.step();

            if (stepper.is_stopped()) {
               next_action = now_us();
               state = FIND_M_END;
               stepper.target_rel_pos(-APPROX_DISTANCE * 1.5);
            }
            break;
               
         case FIND_M_END: // Move until at m_end, then change direction.
            if (not m_end_inv_switch.value()) {
               m_end_pos = stepper.pos();
               state = FIND_O_END;
               stepper.target_rel_pos(APPROX_DISTANCE * 1.5);
            }
            next_action = stepper.step();
            break;

         case FIND_O_END: // Move until at o_end, then 
            if (not o_end_inv_switch.value()) {
               o_end_pos = stepper.pos();
               state = CALIBRATE;
               stepper.target_pos(o_end_pos);
            }
            next_action = stepper.step();
            break;

         case CALIBRATE: // Calibrate the position.
            if (stepper.is_stopped()) {
               o_end_pos = o_end_pos - m_end_pos;
               m_end_pos = 0;
               stepper.calibrate_position(o_end_pos);
               stepper.target_pos(o_end_pos / 2);
               state = CENTER;
            }
            next_action = stepper.step();
            break;

         case CENTER: // Move it to the center.
            if (stepper.is_stopped()) {
               eq.enqueue_now(calibrate_finished);
               return;
            }
            next_action = stepper.step();
            break;
      }
      eq.enqueue(this, next_action);
   }
};
calibrate calibrate_obj(m_end_pos, o_end_pos);

void standby_for_calibrate(event_queue& eq, const timestamp_t& when)
{
   if (start_but.pressed()) {
      y_led_blink.stop();
      g_led.on();
      eq.enqueue_now(&calibrate_obj);
      return;
   }

   eq.enqueue(standby_for_calibrate, when + 100 * MILLIS);
}

void calibrate_finished(event_queue& eq, const timestamp_t& when)
{
   g_led.off();
   y_led_blink.start();
   delay_unitl(stepper.off());
}

void start(event_queue& eq, const timestamp_t& when)
{
   stepper.target_speed(MAX_SPEED);
   stepper.acceleration(MAX_ACCELERATION);
   eq.stop();
}


void loop() {}

