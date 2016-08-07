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
int32_t mid_pos;

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
   eq.enqueue_now(calibrate_standby);
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
   mid_pos = 0;
   
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
   mid_pos = o_end_pos / 2;
   stepper.calibrate_position(o_end_pos);
   stepper.target_pos(mid_pos);
   eq.enqueue_now(calibrate_center);
}

void calibrate_center(event_queue& eq, const timestamp_t& when)
{
   if (not stepper.is_stopped()) {
      eq.enqueue(calibrate_center, stepper.step());
      return;
   }

   serial.p("calirated to ", m_end_pos, " - ", o_end_pos, "\n");
   
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

enum action:uint8_t {
   STILL,    // Waiting for it to be at rest before starting to swing.
   SWING_O,  // We can't balance it, swing it to a balancable position, expected swing is to the other side.
   SWING_M,  // We can't balance it, swing it to a balancable position, expected swing is to the motor side.
   BALANCE,  // It should be balanceable from here.
};

constexpr int32_t UP = 252;
constexpr int32_t DOWN = UP + 512;
constexpr int32_t BALANCE_LO = UP - 128;
constexpr int32_t BALANCE_HI = UP + 128;
constexpr int32_t SWING_LO = DOWN - 128;
constexpr int32_t SWING_HI = DOWN + 128;
constexpr int32_t BAD_LO = 50;
constexpr int32_t BAD_HI = 890;
constexpr timestamp_t TICK = MILLIS * 10;
#define STATE_SIZE 8

// This is the factor to calculate the number of steps that corresponds to an ang for the pendulum. This is
// pendelum_length*sin(ang/1024*2*pi) * steps_per_meter but for small angles sin(x) = x, for simplicity the factor is
// pendelum_length / 1024 * pi * steps_per_meter.
constexpr float LENGTH = 0.12;
constexpr float STEPS_PER_METER = 1240/0.245;
constexpr uint32_t STEPS_PER_ANG = LENGTH / 1024 * 2 * PI * STEPS_PER_METER;

// Helper class for handling state.
struct run_state {

   // Returns true if the pendulum seems to be decelerating. Use last count for calculation, must be an even number.
   bool decelerating(int32_t count = 0)
   {
      if (count == 0) count = STATE_SIZE;
         
      int32_t curr_ang_speed = 0;
      int32_t prev_ang_speed = 0;
      uint8_t i = 0;
      for (; i < count >> 1; ++i) {
         curr_ang_speed += ang_speed(i);
      }
      for (; i < count; ++i) {
         prev_ang_speed += ang_speed(i);
      }

      if (curr_ang_speed < 0 and prev_ang_speed < 0) {
         return prev_ang_speed + count < curr_ang_speed;
      }

      if (0 < curr_ang_speed and 0 < prev_ang_speed) {
         return curr_ang_speed + count < prev_ang_speed;
      }

      return false;
   }
 
   // Returns true if the pendulum seems to be accelerating. Use last count for calculation, must be an even number.
   bool accelerating(int32_t count = 0)
   {
      if (count == 0) count = STATE_SIZE;
      
      int32_t curr_ang_speed = 0;
      int32_t prev_ang_speed = 0;
      uint8_t i = 0;
      for (; i < count >> 1; ++i) {
         curr_ang_speed += ang_speed(i);
      }
      for (; i < count; ++i) {
         prev_ang_speed += ang_speed(i);
      }

      if (curr_ang_speed < 0 and prev_ang_speed < 0) {
         return curr_ang_speed + count < prev_ang_speed;
      }

      if (0 < curr_ang_speed and 0 < prev_ang_speed) {
         return prev_ang_speed + count < curr_ang_speed;
      }

      return false;
   }
  
   uint8_t index;

   uint32_t     tick_count;            // Useful for debug printouts.
   int32_t     _ang_speed[STATE_SIZE]; // Angular speed measured in steps/tick, 1024 steps total.
   int32_t     _ang[STATE_SIZE];       // Position.
   bool        _dead_ang[STATE_SIZE];  // Is the ang in the dead zone?
   bool        dead_zone;              // Are we currently in the dead zone?
   timestamp_t last_measure;           // Last time we did measure.
   int32_t     delta;                  // Best known delta for true down and true up (this changes all the time).

   action      state;                  // Current state of running.
   
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
      tick_count = 0;
      delta = 0;
      state = STILL;
   }

   // Measure everything, and store result in cirkbuf of size STATE_SIZE.
   void measure()
   {
      ++tick_count;
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

      // for (uint8_t i = 0; i < STATE_SIZE; ++i) {
      //    serial.p(i, " ", ang(i), " ", ang_speed(i), "\n");
      // }

      // if (tick_count % 64 == 0) {
      //    serial.p(dead_zone, " ", ang(0), " ", ang_speed(0), "\n");
      // }
   }

   // Return true if pointing up-ish and is slow enough.
   bool can_balance()
   {
      return delta + BALANCE_LO < ang(0) and ang(0) < delta + BALANCE_HI and abs(ang_speed(0)) < 200;
   }
   
   // Return true if pointing down-ish and is slow.
   bool need_swinging()
   {
      return delta + SWING_LO < ang(0) and ang(0) < delta + SWING_HI and abs(ang_speed(0)) < 50;
   }

   inline int32_t down(const int32_t& diff) { return DOWN + delta + diff; }

   // Return true if appears to be still.
   bool still()
   {
      int32_t sum = 0;
      bool still = true;
      for (uint8_t i = 0; i < STATE_SIZE; ++i) {
         sum += ang_speed(i);
         still = still and -3 < ang_speed(i) and ang_speed(i) < 3;
      }
      if (still and -2 < sum and sum < 2) {
         // We are still, let's calibrate delta.
         delta = DOWN - ang(0);
         serial.p("delta ", delta, "\n");
         return true;
      }
      return false;
   }

};

run_state rs;

void run_start(event_queue& eq, const timestamp_t& when)
{
   g_led_blink.start(FAST_BLINK_DELAY);
   y_led_blink.stop();
   y_led.on();
   delay_unitl(stepper.on());
   stepper.target_pos(mid_pos);
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

   int32_t pos = stepper.pos();
   int32_t new_pos = pos;
      
   if (rs.dead_zone) {
      // Never do anything in dead zone.
   }
   else if (rs.state == STILL) {
      
      if (rs.still()) {
         new_pos = 0;
         serial.p("jerk m <= o, next swing o\n");
         rs.state = SWING_O;
      }
   }
   else if (rs.state == SWING_O and stepper.is_stopped() and rs.ang_speed(0) < 0 and rs.ang(0) < rs.down(30)) {
      new_pos = mid_pos + 150;
      serial.p("swing add m => o, next swing m\n");
      rs.state = SWING_M;
   }
   else if (rs.state == SWING_M and stepper.is_stopped() and 0 < rs.ang_speed(0) and rs.down(-30) < rs.ang(0)) {
      new_pos = mid_pos - 150;
      serial.p("swing add m <= o, next swing o\n");
      rs.state = SWING_O;
   }
   else {
      // serial.p("nothing ", rs.ang(0), "\n");
   }
   
   if (new_pos != pos) {
      stepper.target_pos(min(max(new_pos, m_end_pos + 100), o_end_pos - 100));
   };
   
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
