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
   event events[EVENTS_SIZE];

   // Index of front of the queue.
   index_t index;

   // Current number of elements.
   index_t size;

   event_queue() : index(0), size(0)
   {
      for (auto& e : events) {
         e.when = 0;
         e.callback = NULL;
      }
   }
   
   // Returns true if x is before y. Can only used reliably for values in the interval -5 mins to +65 mins from now.
   bool before(const timestamp_t& now, const timestamp_t& x, const timestamp_t& y) {
      constexpr uint32_t _5_MINS = 5 * 60 * 1000 * 1000;
      return uint32_t(x - now + _5_MINS) < uint32_t(y - now + _5_MINS);
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
      while (size) {
         timestamp_t now = now_us();
         if (before(now, now, events[index].when)) {
            auto delay = min(timestamp_t(1000000), events[index].when - now);
            delayMicroseconds(delay);
         }
         else {
            auto cb = events[index].callback;
            auto when = events[index].when;
            --size;
            index = next(index);
            cb(*this, when);
         }
      }
      show_error(error::EVENT_QUEUE_EMPTY);
   }

   // Enqueue event into the event loop, if queue is full it will show error. Depending on what type timestamp_t is it
   // may wrap (70 minutes on arduino uno and teensy32), add to that some lag in handling is also possible so deltas
   // above 60 mins (3.6e9 us) is bad practice.
  void enqueue(callback_t callback, uint32_t when)
   {
      if (size == EVENTS_SIZE) {
         show_error(error::EVENT_QUEUE_FULL);
      }
      else {
         // First insert in and then let it bubble up.

         size++;

         auto front_index = index;
         auto back_index = (index + size - 1) % EVENTS_SIZE;
         
         events[back_index].callback = callback;
         events[back_index].when = when;

         timestamp_t now = now_us();

         while (back_index != front_index) {
            auto back_peek = prev(back_index);

            if (before(now, events[back_peek].when, events[back_index].when)) {
               break;
            }
            
            // Swap index and peek to let it bubble up.
            event e = events[back_index];
            events[back_index] = events[back_peek];
            events[back_peek] = e;

            back_index = back_peek;
         }
      }
   }

   // void print()
   // {
   //    cerr << "size \t" << uint32_t(size) << endl;
   //    cerr << "index \t" << uint32_t(index) << endl;
   //    timestamp_t now = now_us();
   //    for (uint32_t i = 0; i < EVENTS_SIZE; ++i) {
   //       cerr << i << " \t" << events[i].when - now << " \t" << uint64_t(events[i].callback) << endl;
   //    }
   // }
   
};
