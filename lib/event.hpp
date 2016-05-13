#pragma once

//
// Lib for cpp micro controller event loop and callbacks.
//

#include <utility>

#include "error.hpp"

// Size of event queue.
#ifndef EVENTS_SIZE
#define EVENTS_SIZE 10
#endif

struct event_queue;

using callback_t = void (*)(event_queue& event_queue);

// One event in the event loop.
struct event
{

   // Depending on what type timestamp_t is it may wrap (70 minutes on arduino uno and teensy32), add to that some lag
   // in handling is also possible so deltas above 60 mins (3.6e9 us) is bad practice.
   timestamp_t when_us;

   // Callback that will be called at around when_us, at least the order is guaranteed. NULL if this instance is
   // disabled.
   callback_t callback;
   
};

struct event_queue
{

   // This is a sorted circular buffer with the next event first.
   event events[EVENTS_SIZE];

   // Index of front of the queue.
   uint8_t index;

   // Current number of elements.
   uint8_t size;

   event_queue() : index(0), size(0)
   {}
   
   // Run the event queue.
   void run()
   {
      while (size) {
         timestamp_t now = now_us();
         if (now < events[index].when_us and events[index].when_us - now ) {
         }
         
         auto cb = events[index];
         --size;
      }
      show_error(error::EVENT_QUEUE_EMPTY);
   }

   // Returns true if x is before y. Can only be reliably for values in the interval -5 mins to +65 mins from now.
   bool before(const timestamp_t& now, const timestamp_t& x, const timestamp_t& y) {
      
   }

   uint8_t next(uint8_t index) {
      return (index + 1) % EVENTS_SIZE;
   }

   uint8_t prev(uint8_t index) {
      return (index - 1) % EVENTS_SIZE;
   }
   
   // Enqueue event into the event loop, if queue is full it will show error.
   void enqueue(callback_t callback, uint32_t when)
   {
      if (size == EVENTS_SIZE) {
         show_error(error::EVENT_QUEUE_FULL);
      }
      else {
         // First insert in and then let it bubble up.

         auto front_index = index;
         auto back_index = (index + size - 1) % EVENTS_SIZE;
         
         events[back_index].callback = callback;
         events[back_index].when_us = when;

         // Now (- 1 minute) is used to detect wrapping. 
         timestamp_t now = now_us() - 60e6;
         
         auto index = back_index;
         auto peek = prev(back_index);

         while (index != front_index) {

            if (events[peek].when_us - now <= events[index].when_us - now) {
               break;
            }

            // Swap index and peek to let it bubble up.
            std::swap(events[peek], events[index]);
            index = prev(index);
            peek = prev(peek);
         }
      }
   }
   
};
