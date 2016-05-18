#pragma once

//
// Lib for cpp micro controller event loop and callbacks.
//

#include "error.hpp"

// Size of event queue.
#ifndef EVENTS_SIZE
#define EVENTS_SIZE 16
#endif

using namespace std;

struct event_queue
{
   using callback_t = void (*)(event_queue& event_queue, const timestamp_t& when);
   using index_t = uint8_t;

   // One event in the event loop.
   struct event
   {
      
      timestamp_t when;

      callback_t callback;
      
      void operator=(const event& other) {
         when = other.when;
         callback = other.callback;
      }
   };

   // This is a sorted circular buffer with the next event first.
   event _events[EVENTS_SIZE];

   // Index of front of the queue.
   index_t _index;

   // Current number of elements.
   index_t _size;

   bool _run;
   
   event_queue() : _index(0), _size(0), _run(true)
   {
      for (auto& e : _events) {
         e.when = 0;
         e.callback = NULL;
      }
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
            auto cb = _events[_index].callback;
            auto when = _events[_index].when;
            --_size;
            _index = next(_index);
            cb(*this, when);
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
   
   void enqueue_now(callback_t callback)
   {
      enqueue(callback, now_us());
   }
   
   // Enqueue event into the event loop, if queue is full it will show error. Depending on what type timestamp_t is it
   // may wrap (70 minutes on arduino uno and teensy32), add to that some lag in handling is also possible so deltas
   // above 60 mins (3.6e9 us) is bad practice.
   void enqueue(callback_t callback, uint32_t when)
   {
      if (_size == EVENTS_SIZE) {
         show_error(error::EVENT_QUEUE_FULL);
      }
      else {
         // First insert in and then let it bubble up.

         _size++;

         auto front_index = _index;
         auto back_index = (_index + _size - 1) % EVENTS_SIZE;
         
         _events[back_index].callback = callback;
         _events[back_index].when = when;

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

   // void print()
   // {
   //    cerr << "_size \t" << uint32_t(_size) << endl;
   //    cerr << "index \t" << uint32_t(index) << endl;
   //    timestamp_t now = now_us();
   //    for (uint32_t i = 0; i < EVENTS_SIZE; ++i) {
   //       cerr << i << " \t" << _events[i].when - now << " \t" << uint64_t(_events[i].callback) << endl;
   //    }
   // }
   
};
