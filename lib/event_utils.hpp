#pragma once

#include "event_queue.hpp"

struct led_blinker : public event_queue::callback_obj_at
{
   led_blinker(event_queue& event_queue, led& led, const delay_t& interval=SECOND) :
      _event_queue(event_queue), _led(led), _interval(interval), _run(false)
   {}

   void interval(const delay_t& interval) {
      _interval = interval;
   }

   void start(const delay_t& interval) {
      this->interval(interval);
      start();
   }

   void start() {
      if (_run) {
         return;
      }
      _run = true;
      _event_queue.enqueue_now(this);
   }

   void stop() {
      _run = false;
      _led.off();
   }
   
   void operator()(event_queue& eq, const timestamp_t& when) override
   {
      if (_run) {
         _led.toggle();
         eq.enqueue_at(this, when + _interval);
      }
   }
   
private:
   event_queue& _event_queue;
   led& _led;
   delay_t _interval;
   bool _run;
};

