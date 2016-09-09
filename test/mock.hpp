#pragma once

#include <sys/time.h>
#include <unistd.h>

#include <vector>
#include <iostream>
#include <algorithm>

#define OUTPUT 0
#define INPUT 1

#define RISING 0
#define CHANGE 2

using pin_t       = uint8_t;
using pin_value_t = uint8_t;

using ang_t = int16_t;

using delay_t     = uint32_t;
using timestamp_t = uint32_t;

using byte = uint8_t;

std::vector<uint8_t> pin_modes(256);
std::vector<uint16_t> pin_values(256);

void pinMode(uint8_t pin, uint8_t mode)
{
   pin_modes[pin] = mode;
}

void digitalWrite(uint8_t pin, uint8_t value)
{
   pin_values[pin] = value & 1;
}

uint8_t digitalRead(uint8_t pin)
{
   return pin_values[pin] & 1;
}


uint64_t start_us = 0;
uint32_t now_us()
{
   timeval now;
   ::gettimeofday(&now, 0);
   uint64_t now64 = now.tv_sec * 1000000 + now.tv_usec;
   if (start_us == 0) {
      start_us = now64;
   }
   return uint32_t(now64 - start_us);
}

void delayMicroseconds(uint32_t delay) {
   usleep(delay);
}

pin_t digitalPinToInterrupt(pin_t pin) {
   return 0;
}

void attachInterrupt(pin_t pin, void (*func)(void), int mode) {
}
