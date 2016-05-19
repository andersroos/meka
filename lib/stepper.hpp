#pragma once

//
// Stepper motor lib.
//
// Will run the motor to a position (designated by a absolute position in full steps).
//
// WARNING: Timing uses uint32_t for counting us, undefined behaviour if more than 2^32 us (~71 mins) since micro
// controller reset.
//
// WARNING: Will do busy waits for small (~1 us) waits, this is probably a bad idea for fast CPUs (>~100 MHz). Also
// since we don't have nano timestamp we will have to wait 1 us extra all the time (473 to 474 can be 1ns if unlucky).
//

using namespace std;

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

// Max level of micro (0 is also a level, 1 micro step per step). So if the driver can do 32 it should be 5.
#define MAX_MICRO 5

//
// Stepping constants.
//

// This is code dependet, below this delays will be upshifted for precision.
#define SHIFT_THRESHOLD (uint32_t(1) << 30)

// Target speed set from start.
#define DEFAULT_TARGET_SPEED 10.0

// Target acceleration set at start.
#define DEFAULT_ACCEL 10.0

//
// Stepper interface.
//

struct stepper
{

   enum state:uint8_t { OFF, ACCEL, DECEL, TARGET_SPEED };
   
   // Create obj for motor.
   // 
   // forward_value: the value on the dir pin that is forward
   //
   // smooth_delay: the delay where the motor runs well, the code will try to micro step to reach this delay and keep
   //               delays between smooth_delay and smooth_delay / 2, smooth_delay / 2 have to be longer than is needed
   //               for the step calculation or it will accelerate badly
   stepper(pin_t       dir_pin,
           pin_t       step_pin,
           pin_t       enable_pin,
           pin_t       micro0_pin,
           pin_t       micro1_pin,
           pin_t       micro2_pin,
           pin_value_t forward_value,
           delay_t     smooth_delay);
           
   // Set position to pos, requires stopped state.
   void calibrate_position(int32_t pos=0);

   // Turn on power to be able to move or hold. If you start stepping before it is turned on, it will lose track of
   // position and speed and be unable to accelerate properly.
   //
   // returns: timestamp when stepper motor will be turned on
   timestamp_t on();

   // Turn off power. If you do this when the motor is not stopped, it will lose track of its positon.
   //
   // returns: timestamp when stepper motor will be turned off
   timestamp_t off();

   // Set acceleration (and deceleration), this requires stopped state.
   //
   // accel: target acceleration in full steps/second^2
   void acceleration(float accel);

   // Set target position, can be called at any time.
   //
   // pos: target position in absolute steps
   void target_pos(int32_t pos);

   // Set target position relative to current position, can be called at any time.
   //
   // pos: target position relative to current position
   void target_rel_pos(int32_t rel_pos);
   
   // Set target speed, can be called at any time. If changing from a higer to a lower speed the motor will decelrate to
   // that speed.
   //
   // speed: the requested speed in full steps/second
   void target_speed(float speed);

   // Return if the stepper is stopped at the target or if it is turned off.
   bool is_stopped();

   // Step toward target position.
   //
   // returns: timestamp when step is finished and you should call step again to take next step, be as accurate as
   //          possible, returns 0 if arrived at target and speed is 0
   timestamp_t step();
   // TODO This is bad, time can be valid and 0 (sure a hack could be to make sure it never is).

   // Get raw position, note that this is shifted when micro stepping.
   //
   // returns: position
   inline int32_t raw_pos() { return _pos; }
   
   // Get position, will be rounded down if micro stepping.
   //
   // returns: position
   inline int32_t pos() { return _pos >> _micro; }

   // Get current micro stepping level.
   //
   // returns: position
   inline uint8_t micro() { return _micro; }

   // Get last calculated delay.
   //
   // returns: delay in micro seconds
   inline uint32_t delay() { return _return_delay; }
   
   // Get current target position.
   //
   // returns: current target position
   inline uint32_t target_pos() { return _target_pos >> _micro; }

private:

   void shift_down();

   void shift_up();

   void micro_down(uint8_t levels);

   void micro_up(uint8_t levels);

   void micro_set();
   
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
   
   delay_t  _delay0[MAX_MICRO + 1]; // Starting delay (this is acceleration constant), per micro level.
   delay_t  _delay;                 // Current delay (time needed for step to move physically).
   delay_t  _smooth_delay;          // Delay where motor runs smoothly (ideal delay), when to change micro level.
   delay_t  _target_delay;          // This is our target speed.

   uint8_t  _shift;                 // Shift level for precision.

   delay_t  _return_delay;          // Last returned delay, for debugging.

   // Book keping.

   state _state;
};


stepper::stepper(pin_t       dir_pin,
                 pin_t       step_pin,
                 pin_t       enable_pin,
                 pin_t       micro0_pin,
                 pin_t       micro1_pin,
                 pin_t       micro2_pin,
                 pin_value_t forward_value,
                 delay_t     smooth_delay)
   : _dir_pin(dir_pin),
     _step_pin(step_pin),
     _enable_pin(enable_pin),
     _micro0_pin(micro0_pin),
     _micro1_pin(micro1_pin),
     _micro2_pin(micro2_pin),
     _forward_value(forward_value),
     _dir(1),
     _pos(0),                              
     _target_pos(0),
     _accel_steps(0),
     _micro(0),
     _delay(0),
     _smooth_delay(smooth_delay),
     _target_delay(1e6),
     _shift(0),
     _return_delay(0),
     _state(OFF)
{
   pinMode(dir_pin, OUTPUT);
   pinMode(step_pin, OUTPUT);
   pinMode(enable_pin, OUTPUT);
   pinMode(micro0_pin, OUTPUT);
   pinMode(micro1_pin, OUTPUT);
   pinMode(micro2_pin, OUTPUT);
   
   digitalWrite(dir_pin, forward_value);
   digitalWrite(enable_pin, !STEPPER_ENABLE);
   acceleration(DEFAULT_ACCEL);
   target_speed(DEFAULT_TARGET_SPEED);
}

inline bool
stepper::is_stopped()
{
   return _state == OFF or (_pos == _target_pos and _accel_steps == 0);
}

void
stepper::shift_down()
{
   for (uint8_t i = 0; i <= MAX_MICRO; ++i) {
      _delay0[i] >>= _shift;
   }
   _delay >>= _shift;
   _target_delay >>= _shift;
   _smooth_delay >>= _shift;
   _shift = 0;
}

void
stepper::shift_up()
{
   if (_shift != 0) {
      return;
   }
   
   uint32_t max_delay = max(max(_delay0[MAX_MICRO], _target_delay), _smooth_delay);
   while (max_delay < SHIFT_THRESHOLD) {
      _shift += 1;
      max_delay <<= 1;
   }

   for (uint8_t i = 0; i <= MAX_MICRO; ++i) {
      _delay0[i] <<= _shift;
   }
   _delay <<= _shift;
   _target_delay <<= _shift;
   _smooth_delay <<= _shift;
}

inline void
stepper::micro_down(uint8_t levels=1)
{
   _micro -= levels;
   _accel_steps >>= levels;
   _pos >>= levels;
   _target_pos >>= levels;
}

inline void
stepper::micro_up(uint8_t levels=1)
{
   _micro += levels;
   _accel_steps <<= levels;
   _pos <<= levels;
   _target_pos <<= levels;
}

inline void
stepper::micro_set()
{
   digitalWrite(_micro0_pin, _micro >> 0 & 1);
   digitalWrite(_micro1_pin, _micro >> 1 & 1);
   digitalWrite(_micro2_pin, _micro >> 2 & 1);
}

void
stepper::calibrate_position(int32_t pos)
{
   if (!is_stopped()) {
      return;
   }

   shift_down();
   _pos = pos << _micro;
   shift_up();
}

timestamp_t
stepper::on()
{
   uint32_t now = now_us();
   digitalWrite(_enable_pin, STEPPER_ENABLE);
   _state = ACCEL;
   _accel_steps = 0;
   micro_up(MAX_MICRO - _micro);
   micro_set();
   _delay = _delay0[_micro];
   return now + ENABLE_US + 1;
}

timestamp_t
stepper::off()
{
   uint32_t now = now_us();
   digitalWrite(_enable_pin, !STEPPER_ENABLE);
   _state = OFF;
   return now + ENABLE_US + 1;
}

void
stepper::acceleration(float accel)
{
   if (!is_stopped()) {
      return;
   }

   shift_down();

   float d0 = sqrt(1/accel) * 1e6;
   for (uint8_t m = 0; m <= MAX_MICRO; ++m) {
      _delay0[m] = uint32_t(d0 * sqrt(1 << m));
   }
   
   _delay = _delay0[_micro];

   shift_up();
}

inline void
stepper::target_pos(int32_t pos)
{
   _target_pos = pos << _micro;
   _state = ACCEL;
}

inline void
stepper::target_rel_pos(int32_t rel_pos)
{
   target_pos((_pos >> _micro) + rel_pos);
}

void
stepper::target_speed(float speed)
{
   shift_down();
   
   _target_delay = delay_t(1e6 / speed);

   if (!is_stopped()) {
      // Make sure state changes based on target speed if running.
      if (_target_delay > _delay) {
         _state = DECEL;
      }
      else {
         _state = ACCEL;
      }
   }
   
   shift_up();
}

timestamp_t
stepper::step()
{
   delay_t d = max(_delay, _target_delay);
   auto micro = _micro;

   if (_state == ACCEL) {
      while ((_pos & 1) == 0 and _micro > 0 and d < (_smooth_delay << (_micro - 1))) {
         micro_down();
      }
   }
   else {
      while (_micro < MAX_MICRO and d > (_smooth_delay << _micro)) {
         micro_up();
      }
   }

   if (micro != _micro) {
      uint32_t start = now_us();
      micro_set();
      // Busy wait for mode change here, not good but ok.
      
      while (start + MODE_CHANGE_US + 1 >= now_us());
   }

   int32_t distance = _target_pos - _pos;

   // Handle non stepping states (stopped) before stepping.

   if (_accel_steps <= 1 and (_pos & (int32_t(-1) << _micro)) == _pos) {
      // It is possible to stop now if we want to, no need to decelerate more.

      if (distance == 0) {
         // We have arrived, so stop.
         _accel_steps = 0;
         _delay = _delay0[_micro];
         _state = ACCEL;
         _return_delay = 0;
         return _return_delay;
      }
      
      if ((_dir > 0) == (distance < 0)) {
         // Change dir, allow some time for it.
         _accel_steps = 0;
         _dir = -_dir;
         _state = ACCEL;
         if (_dir < 0) {
            digitalWrite(_dir_pin, !_forward_value);
         }
         else {
            digitalWrite(_dir_pin, _forward_value);
         }
         _return_delay = _target_delay >> _shift;
         return now_us() + _return_delay;
      }
   }

   // Step here and calculate delay later since the calculaion is so slow and we want to include that in the step
   // waiting. The stepping wait is the time for the motor/system to make the step mechanically, when the wait is
   // done the step is done. Step down will be done after calculation.

   timestamp_t step_timestamp = now_us();

   digitalWrite(_step_pin, 1);
   _pos += _dir;

   // Stepping state changes, most important rule first.

   if ((_dir < 0) != (distance < 0) or abs(distance) <= _accel_steps) {
      // We are going in the wrong direction. Or we need to break now or we will overshoot.
      _state = DECEL;
   }
   else if (_state == ACCEL and _delay < _target_delay) {
      // We have reached a good speed
      _state = TARGET_SPEED;
   }
   else if (_state == DECEL and _delay >= _target_delay) {
      // Speed changed before but speed is good now.
      _state = TARGET_SPEED;
   }

   // Do it

   if (_state == DECEL) {
      if (_accel_steps <= 1) {
         _accel_steps = 0;
         _delay = _delay0[_micro];
      }
      else {
         _accel_steps -= 1;
         _delay += (_delay * 2) / (4 * _accel_steps - 1);
      }
      _return_delay = _delay >> _micro >> _shift;
   }
   else {
      if (_state == ACCEL) {
         uint32_t delta;
         if (_accel_steps == 0) {
            delta = 0;
            _delay = _delay0[_micro];
         }
         else {
            delta = _delay * 2 / (4 * _accel_steps + 1);
         }
         _accel_steps += 1;
         _delay -= delta;
      }
      _return_delay = max(_delay, _target_delay) >> _micro >> _shift;
   }
   
   // Make sure time have passed, then downstep.

   uint32_t now;   
   while (true) {
      now = now_us();
      if (step_timestamp + STEPPING_PULSE_US + 1 < now) {
         break;
      }
   }
   digitalWrite(_step_pin, 0);

   return max(step_timestamp + _return_delay, now + STEPPING_PULSE_US + 1); // Downstep needs time too.
}

