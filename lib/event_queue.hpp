#pragma once

//
// Lib for cpp micro controller event loop and callbacks.
//

//#include <functional>

#include "error.hpp"

// Size of event queue.
#ifndef EVENTS_SIZE
#define EVENTS_SIZE 16
#endif

using namespace std;

struct event_queue
{
   struct callback_obj {
      virtual void operator()(event_queue& event_queue, const timestamp_t& when) = 0;
   };
   using callback_fun_t = void (*)(event_queue& event_queue, const timestamp_t& when);
   using callback_obj_t = callback_obj*;
   using index_t = uint8_t;

   // One event in the event loop. We need to have both pointer to function and pointer to functor object in parallell
   // since we don't want to use new thus we can't cretate functor objects for function pointers on the fly. It's a bit
   // ugly but it works. (std::function also uses new in some cases.)
   struct event
   {
      timestamp_t    when;
      callback_fun_t fun;
      callback_obj_t obj;
      
      void operator=(const event& other) {
         fun = other.fun;
         obj = other.obj;
         when = other.when;
      }
   };

   // This is a sorted circular buffer with the next event first.
   event _events[EVENTS_SIZE];

   // Index of front of the queue.
   index_t _index;

   // Current number of elements.
   index_t _size;

   bool _run;
   
   event_queue() {
      reset();
   }

   void reset() {
      _index = 0;
      _size = 0;
      _run = true;
   }

   bool running() {
      return _run;
   }
   
   // Get next index.
   index_t next(const index_t& i) {
      return (i + 1) % EVENTS_SIZE;
   }

   // Get prev index.
   index_t prev(const index_t& i) {
      return (i + (EVENTS_SIZE - 1)) % EVENTS_SIZE;
   }

   // Run the event queue.
   void run()
   {
      _run = true;
      while (_size and _run) {
         timestamp_t now = now_us();
         if (before(now, now, _events[_index].when)) {
            auto delay = min(timestamp_t(1000000), _events[_index].when - now);
            delayMicroseconds(delay);
         }
         else {
            auto event = _events[_index];
            --_size;
            _index = next(_index);
            if (event.obj) {
               event.obj->operator()(*this, event.when);
            }
            else {
               event.fun(*this, event.when);
            }
         }
      }

      if (_run) {
         show_error(error::EVENT_QUEUE_EMPTY);
      }
   }

   void stop()
   {
      _run = false;
   }
   
   // Enqueue event into the event loop, if queue is full it will show error. Depending on what type timestamp_t is it
   // may wrap (70 minutes on arduino uno and teensy32), add to that some lag in handling is also possible so deltas
   // above 60 mins (3.6e9 us) is bad practice.
   void enqueue(callback_fun_t callback, uint32_t when) { _enqueue(callback, nullptr, when); }
   void enqueue(callback_obj_t callback, uint32_t when) { _enqueue(nullptr, callback, when); }
   void enqueue_now(callback_fun_t callback) { enqueue(callback, now_us()); }
   void enqueue_now(callback_obj_t callback) { enqueue(callback, now_us()); }

   bool present(const callback_fun_t callback) const
   {
      for (uint32_t i = 0; i < _size; ++i) {
         if (_events[(i + _index) % EVENTS_SIZE].fun == callback) {
            return true;
         }
      }
      return false;
   }

   bool present(const callback_obj_t callback) const
   {
      for (uint32_t i = 0; i < _size; ++i) {
         if (_events[(i + _index) % EVENTS_SIZE].obj == callback) {
            return true;
         }
      }
      return false;
   }
   
private:

   void _enqueue(callback_fun_t fun, callback_obj_t obj,  uint32_t when)
   {
      if (_size == EVENTS_SIZE) {
         show_error(error::EVENT_QUEUE_FULL);
      }
      else {
         // First insert in and then let it bubble up.

         _size++;

         auto front_index = _index;
         auto back_index = (_index + _size - 1) % EVENTS_SIZE;

         auto& e = _events[back_index];
         e.fun = fun;
         e.obj = obj;
         e.when = when;

         timestamp_t now = now_us();

         while (back_index != front_index) {
            auto back_peek = prev(back_index);

            if (before(now, _events[back_peek].when, _events[back_index].when)) {
               break;
            }
            
            // Swap index and peek to let it bubble up.
            event e = _events[back_index];
            _events[back_index] = _events[back_peek];
            _events[back_peek] = e;

            back_index = back_peek;
         }
      }
   }
};

struct led_blinker : public event_queue::callback_obj
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
         eq.enqueue(this, when + _interval);
      }
   }
   
private:
   event_queue& _event_queue;
   led& _led;
   delay_t _interval;
   bool _run;
};

