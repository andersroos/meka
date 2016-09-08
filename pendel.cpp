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

#define EVENT_QUEUE_DEBUG 8

void log(const char* what);

#include "Arduino.h"
#include "lib/base.hpp"
#include "lib/util.hpp"
#include "lib/stepper.hpp"
#include "lib/event_queue.hpp"
#include "lib/event_utils.hpp"
#include "lib/serial.hpp"
#include "lib/rotary_encoder.hpp"
#include "lib/debug.hpp"
#include "pendel_pins.hpp"

#define SMOOTH_DELAY       200
#define MAX_ACCELERATION 60000
#define MAX_SPEED        16000
#define APPROX_DISTANCE  3000

#define START_BUT     G_BUT
#define PAUS_BUT      Y_BUT
#define EMERGENCY_BUT R_BUT

constexpr uint32_t SLOW_BLINK_DELAY = 200 * MILLIS;
constexpr uint32_t FAST_BLINK_DELAY = 100 * MILLIS;
constexpr uint32_t BUTTON_READ_DELAY = 1 * MILLIS;

constexpr uint16_t ENCODER_REV_TICKS = 2048;

event_queue eq;

debug_log<32> debug;

void log(const char* what) {
   debug.log(what);
}

stepper stepper(DIR, STP, EN, M0, M1, M2, DIR_O, SMOOTH_DELAY);

rotary_encoder<ENC_A, ENC_B, ENCODER_REV_TICKS> encoder;

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

   // TODO serial.p(encoder.ang(), " ", encoder.lap(), "\n");
   // TODO delay(100);
   
   if (not start_but.pressed()) {
      eq.enqueue(calibrate_standby, now_us() + BUTTON_READ_DELAY);
      return;
   }

   g_led_blink.start(SLOW_BLINK_DELAY);

   serial.p("calibrating\n");
   
   // We need a fast acceleration to be able to stop when reaching end, but not crazy fast so we miss steps.
   stepper.acceleration(MAX_ACCELERATION);
      
   // As fast as possible but we need to be able to stop before crashing.
   stepper.target_speed(1300);
      
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
      eq.enqueue(run_standby, now_us() + BUTTON_READ_DELAY);
      return;
   }

   serial.p("running\n");

   eq.enqueue_now(run_start);
}

void run_pause(event_queue& eq, const timestamp_t& when)
{
   serial.p("ang ", encoder.ang(),
            "\n");
   
   if (not stepper.is_stopped()) {
      eq.enqueue(run_pause, now_us() + BUTTON_READ_DELAY);
      return;
   }

   if (stepper.is_on()) {
      delay_unitl(stepper.off());
   }
   
   if (not start_but.pressed()) {
      eq.enqueue(run_pause, now_us() + 100 * MILLIS);
      return;
   }
   
   serial.p("resuming\n");
   
   eq.enqueue_now(run_start);
}

constexpr ang_t DEG_360 = ENCODER_REV_TICKS;
constexpr ang_t DEG_180 = DEG_360 / 2;
constexpr ang_t DEG_90 = DEG_180 / 2;
constexpr ang_t DEG_45 = DEG_90 / 2;
constexpr ang_t DEG_22_5 = DEG_45 / 2;
constexpr ang_t DOWN = 0;
constexpr ang_t UP = -DEG_180;

constexpr timestamp_t TICK = MILLIS * 10;
constexpr uint32_t STATE_SIZE = 16;

// Helper class for handling state.
struct run_state {

   uint8_t index;

   uint32_t    tick_count;             // Useful for debug printouts.
   ang_t       _ang_speed[STATE_SIZE]; // Angular speed measured in steps/tick, 1024 steps total.
   ang_t       step_pos; 
   int32_t     step_speed;             // Number of steps since last tick.
   ang_t       _up_ang[STATE_SIZE];     // Position, relative to up.
   ang_t       _down_ang[STATE_SIZE];   // Position, relative to down.
   timestamp_t last_measure;           // Last time we did measure.
   
   run_state() {
      reset();
   }

   inline ang_t& ang_speed(uint8_t i) { return _ang_speed[(index + i) % STATE_SIZE]; }

   inline ang_t& up_ang(uint8_t i) { return _up_ang[(index + i) % STATE_SIZE]; }

   inline ang_t& down_ang(uint8_t i) { return _down_ang[(index + i) % STATE_SIZE]; }

   // Reset encoder in the down position that we assume we are in.
   void calibrate_down()
   {
      encoder.reset();
      reset_state();
      serial.p("calibrated down\n");
   }
   
   // Return true if appears to be still and down. Note, after a reset this will return true even if not still and down.
   bool still()
   {
      for (uint8_t i = 0; i < STATE_SIZE; ++i) {
         if (ang_speed(i) != 0) {
            return false;
         }
      }
      return true;
   }

   bool going_down()
   {
      auto ang = down_ang(0);
      auto ang_speed = this->ang_speed(0);
      
      if (ang < 0 and ang_speed > 0) {
         return true;
      }

      if (ang > 0 and ang_speed < 0) {
         return true;
      }

      return false;
   }
   
   bool going_up()
   {
      auto ang = up_ang(0);
      auto ang_speed = this->ang_speed(0);
      
      if (ang < 0 and 0 < ang_speed) {
         return true;
      }

      if (0 < ang and ang_speed < 0) {
         return true;
      }

      return false;
   }
         
   void reset_state()
   {
      index = STATE_SIZE - 1;
      for (uint32_t i = 0; i < STATE_SIZE; ++i) {
         ang_speed(i) = 0;
         up_ang(i) = -DEG_180;
         down_ang(i) = 0;
      }
   }
   
   void reset()
   {
      reset_state();
      last_measure = 0;
      tick_count = 0;
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
      down_ang(0) = encoder.ang(DOWN);
      up_ang(0) = encoder.ang(UP);
      ang_speed(0) = encoder.rel(down_ang(0), down_ang(1));

      if (last_measure) {
         auto tick_duration = now - last_measure;
         uint32_t diff = abs(int32_t(tick_duration) - int32_t(TICK));
         if (MILLIS < diff * 2) {
            serial.p("warning, tick was ", tick_duration, " us, diff ", diff, " us\n");
         }      
      }
      last_measure = now;
   }

};

run_state rs;

uint32_t wait_for_still_ticks;

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
   wait_for_still_ticks = 0;
   eq.enqueue_now(run_wait_for_still);
   serial.p("waiting for still\n");
}

constexpr char STILL = 'p';
constexpr char BALANCE = 'b';
constexpr char SWING = 'w';
constexpr char MOVE_TO_MIDDLE = 'm';
char state;
char old_state;

void run_wait_for_still(event_queue& eq, const timestamp_t& when)
{
   if (paus_but.pressed()) {
      run(eq, when);
      return;
   }
   
   rs.measure();
   wait_for_still_ticks++;
   
   if (wait_for_still_ticks < 10 or not rs.still()) {
      eq.enqueue(run_wait_for_still, now_us() + TICK);
      return;
   }

   state = STILL;
   rs.calibrate_down();
   eq.enqueue(run, now_us() + TICK);
}

void run(event_queue& eq, const timestamp_t& when)
{
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
   int32_t target = stepper.target_pos();
   int32_t new_target = target;
   ang_t up_ang = rs.up_ang(0);
   ang_t down_ang = rs.down_ang(0);
   ang_t ang_speed = rs.ang_speed(0);
   uint32_t abs_speed = abs(ang_speed);
   const char* what = "noop";

   // serial.p("tick ", rs.tick_count,
   //          ", up_ang ", up_ang,
   //          ", down_ang ", down_ang,
   //          ", ang_speed ", ang_speed,
   //          ", pos ", pos,
   //          ", target ", target,
   //          "\n");
   
   if (abs(up_ang) < DEG_22_5) {
      // Balance it if speed at apex is low enough.
      
      constexpr float LENGTH = 0.16;
      constexpr float STEPS_PER_METER = 1240 / 0.245;
      constexpr float STEPS_PER_ANG = LENGTH / ENCODER_REV_TICKS * 2 * PI * STEPS_PER_METER; // =~ 3.7
      constexpr float ANG_PER_STEP = 1 / STEPS_PER_ANG; // =~ 0.27
      
      constexpr float SPEED_PER_ANG = 4.0 / 70; // =~ 0.057
      constexpr float ANG_PER_SPEED = 1 / SPEED_PER_ANG; // =~ 18

      constexpr uint16_t MAX_BALANCE_ANG_SPEED = DEG_90 / 35;
      
      // Stepper will affect ang_speed, so true_speed is an attempt to calculate ang_speed as it would have been if
      // steper did not move.
      
      float true_speed = ang_speed + rs.step_speed * ANG_PER_STEP;

      if (abs_speed < MAX_BALANCE_ANG_SPEED and true_speed < MAX_BALANCE_ANG_SPEED) {
         state = BALANCE;
         what = "balancing";
      
         // PD Regulation.
      
         constexpr float Kp = 1.000;
         constexpr float Kd = 0.400;
      
         float p_steps = Kp * up_ang * STEPS_PER_ANG;
         float d_steps = Kd * true_speed * ANG_PER_SPEED * STEPS_PER_ANG;

         new_target = pos + p_steps + d_steps;

         // To make it drit to center when near end.
         
         constexpr uint16_t off_mid_distance = 400;
         if (abs(pos - mid_pos) > off_mid_distance) {
            new_target += (pos - mid_pos) * 0.1;
         }
      }
   }
   else if (abs(up_ang) > DEG_45) {
      // Swing it.
      if (state == STILL) {
         state = SWING;
      }
      else if (state == BALANCE) {
         // We just failed while balancing.
         // 1, Move to middle if needed.
         // 2, Swing it back up.
         state = MOVE_TO_MIDDLE;
         what = "move_to_middle";
         stepper.target_speed(MAX_SPEED / 8);
         new_target = mid_pos;
      }
      else if (state == MOVE_TO_MIDDLE) {
         if (stepper.is_stopped()) {
            stepper.target_speed(MAX_SPEED);
            state = SWING;
         }
      }
      else if (state == SWING) {

         if (stepper.is_stopped()) {
            if (rs.going_down() and abs(down_ang) < DEG_90) {
         
               // // Traditional swing.
               // uint32_t swing_dist = 200;
               // if (ang_speed > 0) {
               //    what = "swing m => o";
               //    new_target = mid_pos + swing_dist;
               // }
               // else {
               //    what = "swing m <= o";
               //    new_target = mid_pos - swing_dist;
               // }

               // Experimental swing.
               uint32_t swing_dist = 300 - (abs_speed - 20) * 6;
               if (ang_speed < 0 and pos <= mid_pos) {
                  what = "swing regulated m => o";
                  new_target = mid_pos + swing_dist;
               }
               else if (ang_speed > 0 and pos >= mid_pos) {
                  what = "swing regulated m <= o";
                  new_target = mid_pos - swing_dist;
               }
            }
            else if (ang_speed == 0 and pos == mid_pos) {
               what = "jerk >= o";
               new_target = mid_pos + 400;
            }
         }
      }
   }
   // else if (pos == target and rs.going_up()) {
   //    what = "breaking";
   //    
   //    constexpr float LENGTH = 0.16;
   //    constexpr float STEPS_PER_METER = 1240 / 0.245;
   //    constexpr float STEPS_PER_ANG = LENGTH / 1024 * 2 * PI * STEPS_PER_METER; // =~ 3.7
   //    constexpr float ANG_PER_STEP = 1 / STEPS_PER_ANG; // =~ 0.27
   //    
   //    constexpr float SPEED_PER_ANG = 4.0 / 70; // =~ 0.057
   //    constexpr float ANG_PER_SPEED = 1 / SPEED_PER_ANG; // =~ 18
   //    
   //    // Stepper will affect ang_speed, so true_speed is an attempt to calculate ang_speed as it would have been if
   //    // steper did not move.
   //    
   //    float true_speed = ang_speed + step_speed * ANG_PER_STEP;
   //    
   //    // PID Regulation.
   //    
   //    int32_t rel_ang = ang - UP;
   //    int32_t rel_ang_sum = rs.rel_ang_sum(UP, 4);
   //    
   //    constexpr float Kp = 0.000;
   //    constexpr float Ki = 0.000;
   //    constexpr float Kd = 0.030;
   //    
   //    float p_steps = Kp * rel_ang * STEPS_PER_ANG;
   //    float i_steps = Ki * rel_ang_sum * STEPS_PER_ANG;
   //    float d_steps = Kd * -ang_speed * ANG_PER_SPEED * STEPS_PER_ANG;
   //    
   //    // serial.p("in zone",
   //    //          ", pos ", pos,
   //    //          ", rel_ang ", rel_ang,
   //    //          ", rel_ang_sum ", rel_ang_sum,
   //    //          ", ang_speed ", ang_speed,
   //    //          ", step_speed ", step_speed,
   //    //          ", true_speed ", true_speed,
   //    //          ", p_steps ", p_steps,
   //    //          ", i_steps ", i_steps,
   //    //          ", d_steps ", d_steps,
   //    //          ", m_steps ", m_steps,
   //    //          ", t ", rs.tick_count,
   //    //          "\n");
   //    
   //    new_target = pos + p_steps + i_steps + d_steps;
   //    
   //    in_swing = false;
   // }
   
   if (new_target != target) {
      const char* limited_message = "";
      if (new_target < m_end_pos + 100) {
         limited_message = "hit motor limit";
         new_target = m_end_pos + 100;
      }
      else if (o_end_pos - 100 < new_target) {
         limited_message = "hit other limit";
         new_target = o_end_pos - 100;
      }
      stepper.target_pos(new_target);
      serial.p(what,
               ", pos ", pos,
               // ", target ", target,
               ", new_target ", new_target,
               // " (", new_target - pos, ")",
               " up_ang ", up_ang,
               ", speed ", ang_speed,
               // ", tick ", rs.tick_count,
               ", ", limited_message, "\n");
   };

   if (state != old_state) {
      serial.p("state change ", old_state, " => ", state, "\n");
      old_state = state;
   }
   
   eq.enqueue(run, when + TICK);
}

// Run stepper in its own "thread".
void run_step(event_queue& eq, const timestamp_t& when)
{
   if (not stepper.is_on()) {
      return;
   }

   if (stepper.is_stopped()) {
      eq.enqueue(run_step, now_us() + MILLIS);
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
