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
   struct callback_obj { virtual void operator()(event_queue& event_queue) = 0; };
   struct callback_obj_at { virtual void operator()(event_queue& event_queue, const timestamp_t& when) = 0; };
   
   using callback_fun_at_t = void (*)(event_queue& event_queue, const timestamp_t& when);
   using callback_fun_t = void (*)(event_queue& event_queue);
   using callback_obj_at_t = callback_obj_at*;
   using callback_obj_t = callback_obj*;

   using index_t = uint8_t;

   enum kind_t:uint8_t { OBJ, OBJ_AT, FUN, FUN_AT };
   
   union fun_t {
      callback_obj_t    obj;
      callback_obj_at_t obj_at;
      callback_fun_t    fun;
      callback_fun_at_t fun_at;
   };

   // One event in the event loop. Since we don't want to use new, we can't use functor objects, thus the union fun_t
   // instead. It's a bit ugly but it works. (std::function also uses new in some cases.)
   struct event
   {
      kind_t         kind;
      fun_t          fun;
      timestamp_t    when;

      inline void fun_set(callback_obj_at_t f) { fun.obj_at = f; kind = OBJ_AT; }
      inline void fun_set(callback_obj_t f)    { fun.obj = f;    kind = OBJ; }
      inline void fun_set(callback_fun_at_t f) { fun.fun_at = f; kind = FUN_AT; }
      inline void fun_set(callback_fun_t f)    { fun.fun = f;    kind = FUN; }

      inline bool fun_eq(callback_obj_at_t f) { return kind == OBJ_AT and fun.obj_at == f; }
      inline bool fun_eq(callback_obj_t f)    { return kind == OBJ    and fun.obj == f; }
      inline bool fun_eq(callback_fun_at_t f) { return kind == FUN_AT and fun.fun_at == f; }
      inline bool fun_eq(callback_fun_t f)    { return kind == FUN    and fun.fun == f; }

      inline void operator()(event_queue& eq)
      {
         switch (kind) {
            case OBJ:    fun.obj->operator()(eq); break;
            case OBJ_AT: fun.obj_at->operator()(eq, when); break;
            case FUN:    fun.fun(eq); break;
            case FUN_AT: fun.fun_at(eq, when); break;
         }
      }
      
      void operator=(const event& other) {
         fun = other.fun;
         kind = other.kind;
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
   inline index_t next(const index_t& i) {
      return (i + 1) % EVENTS_SIZE;
   }

   // Get prev index.
   inline index_t prev(const index_t& i) {
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
            event(*this);
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
   template<typename T> inline void enqueue_at(T callback, timestamp_t when) { _enqueue(callback, when); }
   template<typename T> inline void enqueue_rel(T callback, timestamp_t delta) { _enqueue(callback, now_us() + delta); }
   template<typename T> inline void enqueue_now(T callback) { _enqueue(callback, now_us()); }

   template<typename T> bool present(T callback)
   {
      for (uint32_t i = 0; i < _size; ++i) {
         if (_events[(i + _index) % EVENTS_SIZE].fun_eq(callback)) {
            return true;
         }
      }
      return false;
   }
   
private:

   template<typename T>
   void _enqueue(T fun,  uint32_t when)
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
         e.fun_set(fun);
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

