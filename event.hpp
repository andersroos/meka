//
// Lib for cpp micro controller event loop and callbacks.
// 
//

#ifndef MECHATRONICS_EVENT_HPP
#define MECHATRONICS_EVENT_HPP

// Size of event queue.
#ifndef EVENTS_SIZE
#define EVENTS_SIZE 10
#endif

#define ERROR_EMPTY 0b10101010
#define ERROR_FULL  0b10101000

// One event in the event loop.
struct event
{

   // Wraps in 70 minues, some lag in handling is also allowed so deltas above 60 mins (3.6e9 us) is bad practice.
   uint32_t when_us;

   // Callback that will be called at around when_us, at least the order is guaranteed. NULL if this instance is
   // disabled.
   void (*callback)();
   
};

struct event_queue
{

   // This is a sorted circular buffer with the next event first.
   event* events[EVENTS_SIZE];

   // Index of the next event.
   uint8_t next_index;

   // Number of events in the queue.
   uint8_t size;

   // Run the event queue, if the event queue ends up empty, it will error blink.
   void run()
   {
      while (size > 0) {
         
      }
      blink_error(ERROR_EMPTY);
   }

   // Enqueue event into the event loop, if queue is full it will error blink.
   void enqueue(void (*callback)(), uint32_t when)
   {
      blink_error(ERROR_FULL);
   }
   
};

#endif
