#pragma once

#include "serial.hpp"

// A simple container for debugging of messages and later dumping them.
template<uint16_t size>
struct debug_log
{
   debug_log()
   {
      for (uint16_t i = 0; i < size; ++i) {
         _when[i] = 0;
         _what[i] = "empty";
      }
   }

   void log(const char* what)
   {
      for (uint16_t i = size - 1; i > 0; --i) {
         _when[i] = _when[i - 1];
         _what[i] = _what[i - 1];
      }
      _when[0] = now_us();
      _what[0] = what;
   }

   void dump(noblock_serial& s)
   {
      timestamp_t last = now_us();
      s.p("dump at ", last, "\n");
      for (uint16_t i = 0; i < size; ++i) {
         timestamp_t when = _when[i];
         s.p(when, " ", _what[i], " (", last - when, " to next ^)\n");
         last = when;
      }
   }
   
private:
   timestamp_t _when[size];
   const char* _what[size];
};

