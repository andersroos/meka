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
#include "lib/serial.hpp"
#include "pendel_pins.hpp"

#define SMOOTH_DELAY       200
#define MAX_ACCELERATION 50000
#define MAX_SPEED        12000
#define APPROX_DISTANCE  1500

#define START_BUT     G_BUT
#define PAUS_BUT      Y_BUT
#define EMERGENCY_BUT R_BUT

constexpr uint32_t SLOW_BLINK_DELAY = 200 * MILLIS;
constexpr uint32_t FAST_BLINK_DELAY = 100 * MILLIS;
constexpr uint32_t BUTTON_READ_DELAY = 1 * MILLIS;

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

void calibrate_standby(event_queue& eq, const timestamp_t& when);
void calibrate_move_clear_of_m_end(event_queue& eq, const timestamp_t& when);
void calibrate_find_m_end(event_queue& eq, const timestamp_t& when);
void calibrate_find_o_end(event_queue& eq, const timestamp_t& when);
void calibrate_calibrate(event_queue& eq, const timestamp_t& when);
void calibrate_center(event_queue& eq, const timestamp_t& when);

void run_prepare(event_queue& eq, const timestamp_t& when);
void run_standby(event_queue& eq, const timestamp_t& when);
void run_step(event_queue& eq, const timestamp_t& when);
void run_pause(event_queue& eq, const timestamp_t& when);
void run_start(event_queue& eq, const timestamp_t& when);
void run(event_queue& eq, const timestamp_t& when);

void fin_wait(event_queue& eq, const timestamp_t& when);

void check_for_emergency_stop(event_queue& eq, const timestamp_t& when);
void emergency_stop();

noblock_serial serial(&eq);

void setup()
{
   pinMode(POT, INPUT);
}   

void loop()
{
   if (emergency_but.value()) {
      delayMicroseconds(BUTTON_READ_DELAY);
      return;
   }

   serial.pr("resetting\n");
   serial.pr("standby for calibrate\n");
   
   eq.reset();
   
   delay_unitl(stepper.off());

   g_led.on();
   y_led.off();
   r_led.on();
   builtin_led.off();

   eq.enqueue_now(check_for_emergency_stop);
   //eq.enqueue_now(calibrate_standby);
   eq.enqueue_now(run_standby);
   eq.run();
}

void calibrate_standby(event_queue& eq, const timestamp_t& when)
{
   if (not start_but.pressed()) {
      eq.enqueue(calibrate_standby, when + BUTTON_READ_DELAY);
      return;
   }

   g_led_blink.start(SLOW_BLINK_DELAY);

   serial.p("calibrating\n");
      
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

   eq.enqueue_now(run_prepare);
}

void run_prepare(event_queue& eq, const timestamp_t& when)
{
   g_led_blink.stop();
   g_led.on();
   delay_unitl(stepper.off());

   stepper.target_speed(MAX_SPEED);
   stepper.acceleration(MAX_ACCELERATION);

   serial.p("standby for run\n");
   
   eq.enqueue_now(run_standby);
}

void run_standby(event_queue& eq, const timestamp_t& when)
{
   if (not start_but.pressed()) {
      eq.enqueue(run_standby, when + BUTTON_READ_DELAY);
      return;
   }

   serial.p("running\n");

   eq.enqueue_now(run_start);
}

void run_pause(event_queue& eq, const timestamp_t& when)
{
   if (not stepper.is_stopped()) {
      eq.enqueue(run_pause, when + BUTTON_READ_DELAY);
      return;
   }

   if (stepper.is_on()) {
      delay_unitl(stepper.off());
   }
   
   if (not start_but.pressed()) {
      eq.enqueue(run_pause, when + BUTTON_READ_DELAY);
      return;
   }
   
   serial.p("resuming\n");
   
   eq.enqueue_now(run_start);
}

// enum state:uint8_t {
//    SWING,    // We can't balance it, get it to a balancable position.
//    BALANCE,  // It should be balanceable from here.
//    CENTERED  // Focus on centering it.
// };

// ~767 är ned
// ~255 är upp
// ~480 är höger
// > 900 är opålitligt
// död zon mäter ca 860 eller lite vad som helst (vilket är samma som snett neråt vänster, alltså bra grej)
// < 50 är opålitligt

constexpr int32_t UP = 252;
constexpr int32_t DOWN = UP + 250;
constexpr int32_t BALANCE_LO = UP - 128;
constexpr int32_t BALANCE_HI = UP + 128;
constexpr int32_t SWING_LO = DOWN - 128;
constexpr int32_t SWING_HI = DOWN + 128;
constexpr int32_t BAD_LO = 50;
constexpr int32_t BAD_HI = 890;
constexpr timestamp_t TICK = MILLIS * 10;
constexpr uint32_t STATE_SIZE = 8;

// Helper class for handling state.
struct run_state {

// // UP and DOWN are pretty unreliable, so we will try to update them during the run.
// int32_t dynamic_down = DOWN;
// int32_t dynamic_up = UP;

   uint8_t index;
   
   int32_t _ang_speed[STATE_SIZE]; // Angular speed measured in steps/tick, 1024 steps total.
   int32_t _ang[STATE_SIZE];       // Position.
   bool    _dead_ang[STATE_SIZE];  // Is the ang in the dead zone?
   bool dead_zone;                 // Are we currently in the dead zone?
   timestamp_t last_measure;       // Last time we did measure.

   run_state() { reset(); }

   inline int32_t& ang_speed(uint8_t i) { return _ang_speed[(index + i) % STATE_SIZE]; }
   inline int32_t& ang(uint8_t i) { return _ang[(index + i) % STATE_SIZE]; }
   inline bool& dead_ang(uint8_t i) { return _dead_ang[(index + i) % STATE_SIZE]; }
   
   void reset()
   {
      index = STATE_SIZE - 1;
      for (uint32_t i = 0; i < STATE_SIZE; ++i) {
         ang_speed(i) = 0;
         ang(i) = 0;
         dead_ang(i) = true;
      }
      dead_zone = true;
      last_measure = 0;
   }

   void measure()
   {
      --index;
      auto now = now_us();
      ang(0) = analogRead(POT);
      ang_speed(0) = ang(0) - ang(1);
      dead_ang(0) = not (BAD_LO < ang(0) and ang(0) < BAD_HI);
      dead_zone = dead_ang(0) and dead_ang(1) and dead_ang(2);
               
      if (last_measure) {
         auto tick_duration = now - last_measure;
         uint32_t diff = abs(int32_t(tick_duration) - int32_t(TICK));
         if (MILLIS < diff * 2) {
            serial.p("warning, tick was ", tick_duration, " us, diff ", diff, " us\n");
         }      
      }
      last_measure = now;

      serial.p(dead_zone, " ", ang(0), " ", ang_speed(0), "\n");
   }
   
};

run_state rs;

void run_start(event_queue& eq, const timestamp_t& when)
{
   g_led_blink.start(FAST_BLINK_DELAY);
   y_led_blink.stop();
   y_led.on();
   delay_unitl(stepper.on());
   rs.reset();
   if (not eq.present(run_step)) {
      eq.enqueue_now(run_step);
   }
   eq.enqueue_now(run);
}

void run(event_queue& eq, const timestamp_t& when) {

   if (m_end_switch.value() or o_end_switch.value()) {
      emergency_stop();
   }

   if (paus_but.pressed()) {
      g_led_blink.stop();
      g_led.on();
      y_led_blink.start(FAST_BLINK_DELAY);
      serial.p("pausing\n");
      eq.enqueue_now(run_pause);
      return;
   }

   rs.measure();
   
   // last_ang = curr_ang;
   // curr_ang = analogRead(POT);
   // angular_speed = last_ang - curr_ang;
   // int32_t pos = stepper.pos();
   // int32_t new_pos = pos;
   // 
   // if (not (last_ang < BAD_LO or BAD_HI < last_ang or curr_ang < BAD_LO or BAD_HI < curr_ang or 512 < angular_speed)) {
   //    // Range and speed seems fine.
   // 
   //    if (curr_ang < BALANCE_LO or BALANCE_HI < curr_ang) {
   //       // We can't balance from here try to swing it.
   //       
   //       if (SWING_LO < curr_ang and curr_ang < SWING_HI) {
   // 
   //          if (DOWN - 30 < curr_ang and curr_ang < DOWN + 30 and angular_speed == 0) {
   //             // Not swinging at all, make a forcefull jerk to the side.
   //             new_pos = 0;
   //          }
   //          else if (curr_ang < DOWN - 30) {
   //             // Set to swing pos m, a bit of center.
   //             new_pos = (o_end_pos - m_end_pos) / 2 - 150;
   //          }
   //          else if (DOWN + 30 < curr_ang) {
   //             // Set to swing pos o, a bit of center.
   //             new_pos = (o_end_pos - m_end_pos) / 2 + 150;
   //          }
   //       }
   //    }
   // }
   // 
   // if (new_pos != pos) {
   //    stepper.target_pos(min(max(new_pos, m_end_pos + 100), o_end_pos - 100));
   // };
   
   eq.enqueue(run, when + TICK);
}

// Run stepper in its own "thread".
void run_step(event_queue& eq, const timestamp_t& when)
{
   if (not stepper.is_on()) {
      return;
   }

   if (stepper.is_stopped()) {
      eq.enqueue(run_step, when + 20 * MILLIS);
      return;
   }

   eq.enqueue(run_step, stepper.step());
}

void fin_wait(event_queue& eq, const timestamp_t& when) {
   delay_unitl(stepper.off());
   serial.pr(analogRead(POT), '\n');
   eq.enqueue(fin_wait, when + SECOND);
}

void check_for_emergency_stop(event_queue& eq, const timestamp_t& when)
{
   if (emergency_but.value()) {
      emergency_stop();
      return;
   }
   eq.enqueue(check_for_emergency_stop, when + BUTTON_READ_DELAY);
}

void emergency_stop()
{
   delay_unitl(stepper.off());
   builtin_led.on();   
   eq.stop();
   g_led.off();   
   y_led.off();   
   r_led.off();
   serial.clear();
   serial.pr("\nemergency stop issued\n");
}
