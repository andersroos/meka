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
#include "lib/event_utils.hpp"
#include "lib/serial.hpp"
#include "pendel_pins.hpp"

#define SMOOTH_DELAY       200
#define MAX_ACCELERATION 56000
#define MAX_SPEED        12000
#define APPROX_DISTANCE  3000

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
void run_wait_for_still(event_queue& eq, const timestamp_t& when);
void run(event_queue& eq, const timestamp_t& when);

void check_for_emergency_stop(event_queue& eq, const timestamp_t& when);
void emergency_stop();

noblock_serial serial(&eq);

void setup()
{
   pinMode(POT_0, INPUT);
   pinMode(POT_1, INPUT);
}   

uint32_t c = 0;
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

// TODO Calibrate broken if power is off.
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

   serial.p("calibrated to ", m_end_pos, " - ", mid_pos, " - ", o_end_pos, "\n");
   
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

constexpr uint32_t DEG_45 = 128;
constexpr uint32_t DEG_90 = DEG_45 * 2;
constexpr uint32_t MOTOR = 0;
constexpr uint32_t UP = DEG_90;
constexpr uint32_t OTHER = DEG_90 * 2;
constexpr uint32_t DOWN = DEG_90 * 3;
constexpr uint32_t ANG_MASK = 0x03FF;
constexpr int32_t BALANCE_LO = UP - 120;
constexpr int32_t BALANCE_HI = UP + 120;
constexpr int32_t SWING_LO = DOWN - DEG_45;
constexpr int32_t SWING_HI = DOWN + DEG_45;
constexpr uint32_t DEAD_ZONE_OTHER_LO = 400; // When one of the pots are in this range
constexpr uint32_t DEAD_ZONE_OTHER_HI = 650; // the other pot is in dead zone.

constexpr timestamp_t TICK = MILLIS * 10;
#define STATE_SIZE 16

// This is the factor to calculate the number of steps that corresponds to an ang for the pendulum. This is
// pendelum_length*sin(ang/1024*2*pi) * steps_per_meter but for small angles sin(x) = x, for simplicity the factor is
// pendelum_length / 1024 * pi * steps_per_meter.

// Helper class for handling state.
struct run_state {

   uint8_t index;

   uint32_t     a0;                    // Latest raw angle measurement.      
   uint32_t     a1;                    // Latest raw angle measurement.      
   uint32_t    tick_count;             // Useful for debug printouts.
   int32_t     _ang_speed[STATE_SIZE]; // Angular speed measured in steps/tick, 1024 steps total.
   int32_t     step_pos; 
   int32_t     step_speed;             // Number of steps since last tick.
   uint32_t    _ang[STATE_SIZE];       // Calculated position, see above for positions.
   timestamp_t last_measure;           // Last time we did measure.
   uint32_t    delta0;                 // Add to get true ang from measurement (this changes all the time).
   uint32_t    delta1;                 // Add to get true ang from measurement (this changes all the time).
   
   run_state() { reset(); }

   inline int32_t& ang_speed(uint8_t i) { return _ang_speed[(index + i) % STATE_SIZE]; }
   inline uint32_t& ang(uint8_t i) { return _ang[(index + i) % STATE_SIZE]; }

   // Given that pendulum is down, set delta_0 and delta_1.
   void calibrate_down()
   {
      delta0 = (DOWN - a0) & ANG_MASK;
      delta1 = (DOWN - a1) & ANG_MASK;
      serial.p("calibrated down, delta0 ", delta0, ", delta1 ", delta1, " a0 ", a0, " a1 ", a1, "\n");      
   }
   
   // Return true if appears to be still and down.
   bool still()
   {
      if (not (DOWN - DEG_45 < a0 and a0 < DOWN + DEG_45)) {
         // Calibration can only go so far.
         return false;
      }
      
      int32_t sum = 0;
      uint8_t nonzero = 0;
      for (uint8_t i = 0; i < STATE_SIZE; ++i) {
         sum += ang_speed(i);
         if (ang_speed(i) != 0) {
            ++nonzero;
         }
      }
      if (nonzero < 6 and sum == 0) {
         return true;
      }
      // serial.p("still ", nonzero, " ", sum, "\n");
      return false;
   }

   int32_t rel_ang_sum(int32_t rel, uint8_t count) {
      int32_t sum = 0;
      for (uint8_t i = 0; i < count; ++i) {
         sum += ang(i) - rel;
      }
      return sum;
   }
   
   void reset()
   {
      index = STATE_SIZE - 1;
      for (uint32_t i = 0; i < STATE_SIZE; ++i) {
         ang_speed(i) = 0;
         ang(i) = 0;
      }
      a0 = 0;
      a1 = 0;
      last_measure = 0;
      tick_count = 0;
      delta0 = 0;
      delta1 = 512;
   }

   // Measure everything, and store result in cirkbuf of size STATE_SIZE.
   void measure()
   {
      ++tick_count;
      --index;
      auto now = now_us();

      auto p = stepper.pos();
      step_speed = p - step_pos;
      step_pos = p;
      
      // Do the measurement.
      a0 = analogRead(POT_0);
      a1 = analogRead(POT_1);
      if (DEAD_ZONE_OTHER_LO < a0 and a0 < DEAD_ZONE_OTHER_HI) {
         // 1 in dead zone, use only 0.
         ang(0) = (a0 + delta0) & ANG_MASK;
      }
      else if (DEAD_ZONE_OTHER_LO < a1 and a1 < DEAD_ZONE_OTHER_HI) {
         // 0 in dead zone, use only 1.
         ang(0) = (a1 + delta1) & ANG_MASK;
      }
      else {
         // No one in dead zone, use avarage.
         auto aa0 = (a0 + delta0) & ANG_MASK;
         auto aa1 = (a1 + delta1) & ANG_MASK;
         if ((aa0 < aa1 ? aa1 - aa0 : aa0 -aa1) > 512) {
            ang(0) = (((aa0 + aa1 + 1024) >> 1)) & ANG_MASK;
         }
         else {
            ang(0) = (aa0 + aa1) >> 1;
         }
      }
      ang_speed(0) = ang(0) - ang(1);
      
      if (last_measure) {
         auto tick_duration = now - last_measure;
         uint32_t diff = abs(int32_t(tick_duration) - int32_t(TICK));
         if (MILLIS < diff * 2) {
            serial.p("warning, tick was ", tick_duration, " us, diff ", diff, " us\n");
         }      
      }
      last_measure = now;
   }

   // Return true if pointing up-ish and is slow enough.
   bool can_balance()
   {
      return BALANCE_LO < ang(0) and ang(0) < BALANCE_HI and abs(ang_speed(0)) < 200;
   }
   
   // Return true if pointing down-ish and is slow.
   bool need_swinging()
   {
      return SWING_LO < ang(0) and ang(0) < SWING_HI and abs(ang_speed(0)) < 50;
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
   eq.enqueue_now(run_wait_for_still);
   serial.p("waiting for still\n");
}

void run_wait_for_still(event_queue& eq, const timestamp_t& when) {
   rs.measure();
   
   if (not rs.still()) {
      eq.enqueue(run_wait_for_still, when + TICK);
      return;
   }

   rs.calibrate_down();
   eq.enqueue(run, when + TICK);
}

bool in_swing = false;
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
   int32_t step_speed = rs.step_speed;
   int32_t target = stepper.target_pos();
   int32_t new_target = target;
   uint32_t ang = rs.ang(0);
   int32_t  ang_speed = rs.ang_speed(0);
   const char* what = "noop";

   // Swing it to a balancable position.
   if (SWING_LO < ang and ang < SWING_HI) {
      uint32_t speed = abs(ang_speed);
      uint32_t swing_dist;
      
      if (speed < 25)
         swing_dist = 200;
      else if (speed < 30)
         swing_dist = 120;
      else
         swing_dist = 0;
      
      if (DOWN <= ang) {
         what = "swing m => o";
         new_target = mid_pos + swing_dist;
      }
      else if (ang < DOWN) {
         what = "swing m <= o";
         new_target = mid_pos - swing_dist;
      }
      
      if (not in_swing) {
         serial.p("=== pass swing ===\n");
         in_swing = true;
      }
   }
   else if (BALANCE_LO < ang and ang < BALANCE_HI) {
      what = "balancing";
      
      in_swing = false;
      
      constexpr float LENGTH = 0.16;
      constexpr float STEPS_PER_METER = 1240 / 0.245;
      constexpr float STEPS_PER_ANG = LENGTH / 1024 * 2 * PI * STEPS_PER_METER; // =~ 3.7
      constexpr float ANG_PER_STEP = 1 / STEPS_PER_ANG; // =~ 0.27

      constexpr float SPEED_PER_ANG = 4.0 / 70; // =~ 0.057
      constexpr float ANG_PER_SPEED = 1 / SPEED_PER_ANG; // =~ 18

      // Stepper will affect ang_speed, so true_speed is an attempt to calculate ang_speed as it would have been if
      // steper did not move.

      float true_speed = ang_speed + step_speed * ANG_PER_STEP;
      
      // PID Regulation.

      int32_t rel_ang = ang - UP;
      int32_t rel_ang_sum = rs.rel_ang_sum(UP, 4);

      constexpr float Kp = 1.000;
      constexpr float Ki = 0.070;
      constexpr float Kd = 0.110;

      float p_steps = Kp * rel_ang * STEPS_PER_ANG;
      float i_steps = Ki * rel_ang_sum * STEPS_PER_ANG;
      float d_steps = Kd * true_speed * ANG_PER_SPEED * STEPS_PER_ANG;
      
      // serial.p("in zone",
      //          ", pos ", pos,
      //          ", rel_ang ", rel_ang,
      //          ", rel_ang_sum ", rel_ang_sum,
      //          ", ang_speed ", ang_speed,
      //          ", step_speed ", step_speed,
      //          ", true_speed ", true_speed,
      //          ", p_steps ", p_steps,
      //          ", i_steps ", i_steps,
      //          ", d_steps ", d_steps,
      //          ", m_steps ", m_steps,
      //          ", t ", rs.tick_count,
      //          "\n");
      
      new_target = pos + p_steps + i_steps + d_steps;
   }
   else {
      in_swing = false;
   }
   
   if (new_target != target) {
      const char* limited_message = "";
      if (new_target < m_end_pos + 100) {
         limited_message = ", hit motor limit";
         new_target = m_end_pos + 100;
      }
      else if (o_end_pos - 100 < new_target) {
         limited_message = ", hit other limit";
         new_target = o_end_pos - 100;
      }
      stepper.target_pos(new_target);
      // serial.p(what, ", pos ", pos, ", target ", target, " => ", new_target,
      //          " (", new_target - pos, "), ang ", ang, ", speed ", ang_speed,
      //          limited_message, "\n");
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
      eq.enqueue(run_step, when + MILLIS);
      return;
   }

   eq.enqueue(run_step, stepper.step());
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
