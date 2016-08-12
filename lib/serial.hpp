#pragma once

//
// Better interface for serial communication, uses Arduino Serial internally.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "event_queue.hpp"


#ifndef SERIAL_BUF_SIZE
#define SERIAL_BUF_SIZE 1024
#endif

#define TMP_BUF_SIZE 32

struct noblock_serial : event_queue::callback_obj
{

   noblock_serial(event_queue* eq=nullptr, uint32_t baud_rate=9600) :
      _event_queue(eq), _head(_buf), _tail(_buf), _size(0), _wait(1e6 * 10 / baud_rate)
   {
      Serial.begin(baud_rate);
   }

   // Returns true if bot hw send buffer and sw send buffers are empty.
   bool tx_empty()
   {
      return Serial.availableForWrite() == 0 and _head == _tail;
   }

   // Clear printing buffers, can be nice to use for high priority messages.
   void clear()
   {
      _head = _buf;
      _tail = _buf;
   }
   
   // Print message on serial, not blocking, stops adding parameters when buffer is full.
   template<typename T, typename... Rest>
   noblock_serial& p(T m, Rest... rest)
   {
      char buf[TMP_BUF_SIZE];

      const char* result = _p(buf, m);
      auto len = strlen(result); // strnlen not available?

      if (_size == 0 and len <= Serial.availableForWrite()) {
         // Print directly into hw buffer.
         Serial.print(result);
      }
      else if (len < SERIAL_BUF_SIZE - _size) {
         // Add result to sw buffer and make sure we are in the event queue.
         if (_tail - _buf + len < SERIAL_BUF_SIZE) {
            // Simple case, copy into buf.
            strncpy(_tail, result, len);
            _tail += len;
            _size += len;
         }
         else {
            // Copy in two steps (spanning the buffer end).
            uint32_t len1 = SERIAL_BUF_SIZE - (_tail - _buf);
            strncpy(_tail, result, len1);
            strncpy(_buf, result + len1, len - len1);
            _tail = _buf + len - len1;
            _size += len;
         }

         if (not _event_queue->present(this)) {
            if (not _event_queue->running()) {
               // If possible, print a warning if the event queue is not running.
               if (1 <= Serial.availableForWrite()) {
                  Serial.print("¤");
               }
            }
            _event_queue->enqueue_now(this);
         }
      }
      else {
         // All buffers are full, not more printing.
         return p();
      }

      return p(rest...);
   }

   noblock_serial& p() { return *this; }
   
   // Print into hw buffer directly (like Serial.print), may block if hw buffer is not big enough.
   template<typename T, typename... Rest>
   noblock_serial& pr(T m, Rest... rest)
   {
      Serial.print(m);
      return pr(rest...);
   }

   virtual void operator()(event_queue& event_queue, const timestamp_t& when)
   {
      if (_size == 0) {
         return;
      }

      int32_t copy = min(Serial.availableForWrite(), int32_t(_size));
      
      if (copy > 0) {
         if (SERIAL_BUF_SIZE < _head - _buf + copy) {
            copy = SERIAL_BUF_SIZE - (_head - _buf);
         }
         Serial.write(_head, copy);
         _head += copy;
         _size -= copy;
         if (SERIAL_BUF_SIZE <= _head - _buf) {
            _head = _buf;
         }
      }

      _event_queue->enqueue(this, when + _wait);
   }
   
   noblock_serial& pr() { return *this; }

   virtual ~noblock_serial()
   {
      Serial.end();
   }
   
private:

   const char* _p(char* buf, bool m)
   {
      if (m) {
         return "true";
      }
      return "false";
   }
   
   const char* _p(char* buf, char m)
   {
      buf[0] = m;
      buf[1] = '\0';
      return buf;
   }
   
   const char* _p(char* buf, long m)
   {
      // Print the string backwards.
      char* r = buf + TMP_BUF_SIZE;
      *--r = '\0';

      bool neg = false;
      if (m < 0) {
         neg = true;
         m = -m;
      }

      if (m == 0) {
         *--r = '0';
      }
      else {
         while (m > 0 and r > buf) {
            int div = m / 10;
            int rest = m - div * 10;
            *--r = '0' + rest;
            m = div;
         }
      }

      if (neg) {
         *--r = '-';
      }
      
      return r;
   }

   const char* _p(char* buf, float m)
   {
      bool neg = false;
      if (m < 0) {
         neg = true;
         m = -m;
      }
      
      // As a quick hack for now print as a fixed point number with 3 decimals.
      auto r = const_cast<char*>(_p(buf, long(m * 1000)));
      
      // Insert decimal point and zeroes if needed.
      char* p = r;
      while (p >= buf + TMP_BUF_SIZE - 4) {
         *--p = '0';
      }
      
      memmove(p - 1, p, (buf + TMP_BUF_SIZE - 4) - p);
      buf[TMP_BUF_SIZE - 5] = '.';
      --p;
      
      if (neg) {
         *--p = '-';
      }
      
      return p;
   }

   const char* _p(char* buf, int m)
   {
      return _p(buf, long(m));
   }

   const char* _p(char* buf, unsigned long m)
   {
      return _p(buf, long(m));
   }
   
   const char* _p(char* buf, const char* m)
   {
      return m;
   }

   event_queue* _event_queue;
   char _buf[SERIAL_BUF_SIZE];

   // Head and tail of the buffer. As long as _size > 0 we enqueue checking of the buffer.
   char* _head;
   char* _tail;
   uint32_t _size;

   // Event queue wait time.
   uint32_t _wait;
};
