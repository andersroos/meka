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
      _index = 0;
   }

   void log(const char* what)
   {
      _index = uint16_t(_index - 1) % size;
      _when[_index] = now_us();
      _what[_index] = what;
   }

   void dump(noblock_serial& s)
   {
      timestamp_t last = now_us();
      s.pr("dump at ", last, "\n");
      for (uint16_t i = 0; i < size; ++i) {
         auto index = (i + _index) % size;
         timestamp_t when = _when[index];
         s.pr(when, " ", _what[index], " (", last - when, " to next ^)\n");
         last = when;
      }
   }
   
private:
   timestamp_t _when[size];
   const char* _what[size];
   uint16_t _index;
};

