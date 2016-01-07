//
// Stepper motor lib.
//
// Will run the motor to a position (designated by a absolute position in full steps).
//
// WARNING: Timing uses uint32_t for counting us, undefined behaviour if more than 2^32 us (~71 mins) since micro
// controller reset.
//
// NOTE: Does not support micro stepping, but will.

#include <algorithm>

#ifndef MECHATRONICS_STEPPER_MOTOR_HPP
#define MECHATRONICS_STEPPER_MOTOR_HPP

#define STEPPER_ENABLE 0

// Time needed for driver to change mode (direction, or microstepping mode).
#define MODE_CHANGE_US 2

// Minimal pulse duration for stepping.
#define STEPPING_PULSE_US 1

typedef int16_t pos_t;
typedef int16_t speed_t;

struct stepper
{
   stepper(pin_t dir_pin,
           pin_t step_pin,
           pin_t enable_pin,
           uint8_t enable_value,
           uint8_t forward_value)
      : _dir_pin(dir_pin),
        _step_pin(step_pin),
        _enable_pin(enable_pin),
        _forward_value(forward_value),
        _dir(forward_value),
        _pos(0),
        _speed(0)
   {
      pinMode(dir_pin, OUTPUT);
      pinMode(step_pin, OUTPUT);
      pinMode(enable_pin, OUTPUT);
      digitalWrite(dir_pin, _forward_value);
      digitalWrite(step_pin, 0);
      digitalWrite(enable_pin, !STEPPER_ENABLE);
   }
           
//   // Step motor one step (may do multiple fractions of steps), try to get it to target pos as fast as possible using
//   // parameters (speed and pos are in and out parameters). Caller is responsible for max and min pos (if you want to
//   // emergency stop, use a big accel value).
//   void step_one(speed_t& speed, pos_t& pos, const pos_t& target_pos, const speed_t& max_speed, const speed_t& accel)
//   {
//      pos_t distance = target_pos - pos;
//      if (speed == 0 and distance == 0) {
//         // We have arrived, do nothing.
//         return;
//      }
//      if ((speed > 0 and distance > 0) or (speed < 0 and distance < 0)) {
//         // We are going in the right direction.
//      }
//      else if (
//         }
   
   // Step toward target pos.
   //
   // returns: the microsecond since startup to wait before calling step again, this is not checked, be as accurate as
   //          possible, returns 0 if arrived at target and speed is 0
   uint32_t step() {
   }

   // Reset position to 0 (need to be stopped or thing will go bad).
   void reset() {
      _pos = 0;
   };

   // Turn on power.
   void on() {
      digitalWrite(enable_pin, STEPPER_ENABLE);
   }

   // Turn off power.
   void off() {
      digitalWrite(enable_pin, !STEPPER_ENABLE);
   }

   // Position to go to. Can be changed at any time.
   pos_t   target_pos;

   // Acceleration to use, measured in steps per second. Linar acceleration will be used. Can be changed at any time.
   speed_t accel;

   // Maximum speed to accelerate to. Can be changed at any time.
   speed_t max_speed;

private:
   
   pin_t    _dir_pin;
   pin_t    _step_pin;
   pin_t    _enable_pin;
   uint8_t  _enable_value;
   uint8_t  _forward_value;
   
   uint8_t  _dir;
   pos_t    _pos;
   speed_t  _speed;
   uint32_t _last_step_timestamp;

};

   
#endif
