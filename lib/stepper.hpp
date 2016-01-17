//
// Stepper motor lib.
//
// Will run the motor to a position (designated by a absolute position in full steps).
//
// WARNING: Timing uses uint32_t for counting us, undefined behaviour if more than 2^32 us (~71 mins) since micro
// controller reset.
//

#pragma once

#include <algorithm>

//
// Driver constants.
//

// Enable value.
#define STEPPER_ENABLE 0

// Time needed for driver to change mode (direction, or microstepping mode).
#define MODE_CHANGE_US 1

// Time needed for driver to enable.
#define ENABLE_US 1

// Minimal pulse duration for stepping.
#define STEPPING_PULSE_US 1

// Number of micro levels including level 0 (1 micro step per step). So if the driver can do 32 it should be 6.
#define MICRO_LEVELS 6

//
// Stepping constants.
//

// This is code dependet, below this delays will be upshifted for precision.
#define SHIFT_THRESHOLD (1 << 30)

// Target speed set from start.
#define DEFAULT_TARGET_SPEED 10.0

// Target acceleration set at start.
#define DEFAULT_TARGET_ACCEL 10.0

//
// Stepper interface.
//

typedef uint8_t  pin_t;
typedef uint8_t  pin_value_t;

typedef uint32_t delay_t;
typedef uint32_t timestamp_t;

struct stepper
{

   enum state { ACCEL, DECEL, TARGET_SPEED };
   
   // Create obj for motor.
   // 
   // forward_value: the value on the dir pin that is forward
   stepper(pin_t       dir_pin,
           pin_t       step_pin,
           pin_t       enable_pin,
           pin_t       micro0_pin,
           pin_t       micro1_pin,
           pin_t       micro2_pin,
           pin_value_t forward_value,
           delay_t     smooth_delay);
           
   // Reset position to 0, requires stopped state.
   void reset();

   // Turn on power to be able to move or hold.
   //
   // returns: timestamp when stepper motor will be turned on
   timestamp_t on();

   // Turn off power.
   //
   // returns: timestamp when stepper motor will be turned on
   timestamp_t off();

   // Set acceleration (and deceleration), this requires stopped state.
   //
   // accel: target acceleration in full steps/second^2
   void acceleration(float accel);

   // Set target position, can be called at any time.
   //
   // pos: target position in absolute steps
   void target_pos(int32_t pos);

   // Set target speed, can be called at any time. If changing from a higer to a lower speed the motor will decelrate to
   // that speed.
   //
   // speed: the requested speed in full steps/second
   void target_speed(float speed);
   
   // Step toward target position.
   //
   // returns: timestamp when step is finished and you should call step again to take next step, be as accurate as
   //          possible, returns 0 if arrived at target and speed is 0
   timestamp_t step();

private:

   // Pins and pin values.
   
   pin_t       _dir_pin;
   pin_t       _step_pin;
   pin_t       _enable_pin;
   pin_t       _micro0_pin;
   pin_t       _micro1_pin;
   pin_t       _micro2_pin;
   
   pin_value_t _forward_value;

   // Position and stepping.
   
   int8_t   _dir;          // Current direction 1/-1.
   int32_t  _pos;          // Absolute position.
   int32_t  _target_pos;   // Direction we are moving to.
   uint32_t _accel_steps;  // Number of steps we have accelerated.
   
   uint8_t  _micro;        // Micro stepping level.

   // Delays.
   
   delay_t  _delay0[MICRO_LEVELS];  // Starting delay (this is acceleration constant), per micro level.
   delay_t  _delay;                 // Current delay (time needed for step to move physically).
   delay_t  _smooth_delay;          // Delay where motor runs smoothly (ideal delay), when to change micro level.

   uint8_t  _shift;                 // Shift level for precision.

   // Book keping.

   state _state;

};
