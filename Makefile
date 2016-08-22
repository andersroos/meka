MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BASE_DIR := $(realpath $(CURDIR)/$(MAKEFILE_DIR))

ARDUINO_IDE_DIR ?= $(HOME)/opt/arduino-1.6.6

PORT ?= /dev/ttyACM0

#
# Common variables and settings.
#

CXXFLAGS = -c -g -std=c++11 -Wall -Os

BAUD_RATE = 9600

BOARD = ArduinoUno
#BOARD = Teensy32

#MAIN = pendel
#MAIN = pendel_trial
#MAIN = stepper_changing_speed_trial
#MAIN = stepper_speed_trial
#MAIN = stepper
MAIN = rotary_encoder

EXTRA_OBJS = # $(ARDUINO_DIR)/libraries/Servo/src/avr/Servo.o

default: build

#
# Test/simulation.
#

TEST_CXX = g++

TEST_OBJS = test/run_tests.o

TEST_CXXFLAGS  = -I.

TEST_LIBS = -lboost_unit_test_framework

test/%.o: test/%.cpp $(call rwildcard, $(BASE_DIR)/, *.hpp)
	$(TEST_CXX) $(CXXFLAGS) $(TEST_CXXFLAGS) -c -o $@ $<

test: $(TEST_OBJS)
	$(TEST_CXX) -std=c++11 -o ./run-tests $(TEST_OBJS) $(TEST_LIBS)
	./run-tests

simulate: test/simulate.o
	$(TEST_CXX) -std=c++11 -o $@ $<

#
# Build for micro controller.
#

include $(BOARD).mk

build: $(MAIN).hex

install: # Provided in include file.

#
# Common targets.
# 

console:
	until $$(stty -F $(PORT) cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts); do sleep 0.2; done
	cat $(PORT)

clean:
	\rm -f *.o test/*.o *.elf *.hex *~ *.eef *.eep run-tests simulate

depend:
	makedepend -Y test/*.cpp test/*.hpp lib/*.hpp *.cpp

.PRECIOUS: %.o %.elf
# DO NOT DELETE

test/run_tests.o: test/util_test.hpp test/mock.hpp lib/util.hpp
test/run_tests.o: test/event_queue_test.hpp lib/event_queue.hpp lib/error.hpp
test/run_tests.o: test/stepper_test.hpp lib/stepper.hpp
test/simulate.o: test/mock.hpp lib/stepper.hpp
test/event_queue_test.o: test/mock.hpp lib/util.hpp lib/event_queue.hpp
test/event_queue_test.o: lib/error.hpp
test/stepper_test.o: test/mock.hpp lib/util.hpp lib/stepper.hpp
test/util_test.o: test/mock.hpp lib/util.hpp
lib/event_queue.o: lib/error.hpp
lib/serial.o: lib/event_queue.hpp lib/error.hpp
pendel.o: lib/base.hpp lib/util.hpp lib/stepper.hpp lib/event_queue.hpp
pendel.o: lib/error.hpp lib/serial.hpp lib/event_queue.hpp pendel_pins.hpp
pendel_trial.o: lib/base.hpp lib/util.hpp lib/stepper.hpp pendel_pins.hpp
stepper_changing_speed_trial.o: lib/base.hpp lib/util.hpp lib/stepper.hpp
stepper_speed_trial.o: lib/base.hpp lib/stepper.hpp
