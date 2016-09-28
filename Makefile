MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BASE_DIR := $(realpath $(CURDIR)/$(MAKEFILE_DIR))

#
# Default variables and settings.
#

ARDUINO_IDE_DIR = $(HOME)/opt/arduino-1.6.6

PORT = /dev/ttyACM0

CXXFLAGS = -c -g -std=c++11 -Wall -Os -I$(BASE_DIR)

BAUD_RATE = 9600

EXTRA_OBJS = # $(ARDUINO_DIR)/libraries/Servo/src/avr/Servo.o

default: build

#
# Project.
#

include pendel/make.mk
# include lab/make.mk
# include dev-stepper/make.mk
# include dev-rotary-encoder/make.mk

#
# Test.
#

TEST_CXX = g++

TEST_OBJS = lib/test/run_tests.o

TEST_CXXFLAGS  = -I.

TEST_LIBS = -lboost_unit_test_framework

lib/test/%.o: lib/test/%.cpp
	$(TEST_CXX) $(CXXFLAGS) $(TEST_CXXFLAGS) -c -o $@ $<

test: $(TEST_OBJS)
	$(TEST_CXX) -std=c++11 -o ./lib/test/run-tests $(TEST_OBJS) $(TEST_LIBS)
	./lib/test/run-tests

#
# Build for micro controller.
#

include $(BOARD).mk

build: $(MAIN).hex

#
# Common targets.
# 

console:
	until $$(stty -F $(PORT) cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts); do sleep 0.2; done
	cat $(PORT)

clean:
	\rm -f lib/test/run-tests Makefile.bak dev-stepper/simulate
	find . -name "*.o" -o -name "*.hex" -o -name "*.elf" -o -name "*.eep" -o -name "*.eef" | xargs \rm -f 

depend:
	makedepend -Y */*.hpp */*.cpp */*/*.hpp */*/*.cpp

.PRECIOUS: %.o %.elf
# DO NOT DELETE

lib/debug.o: lib/serial.hpp lib/event_queue.hpp lib/error.hpp
lib/event_queue.o: lib/error.hpp
lib/event_utils.o: lib/event_queue.hpp lib/error.hpp
lib/serial.o: lib/event_queue.hpp lib/error.hpp
dev-stepper/stepper_changing_speed_trial.o: lib/base.hpp lib/util.hpp
dev-stepper/stepper_changing_speed_trial.o: lib/stepper.hpp
dev-stepper/stepper_simple_move.o: lib/base.hpp lib/stepper.hpp
dev-stepper/stepper_speed_trial.o: lib/base.hpp lib/stepper.hpp
pendel/pendel.o: lib/base.hpp lib/util.hpp lib/stepper.hpp
pendel/pendel.o: lib/event_queue.hpp lib/error.hpp lib/event_utils.hpp
pendel/pendel.o: lib/event_queue.hpp lib/serial.hpp lib/rotary_encoder.hpp
pendel/pendel.o: lib/debug.hpp lib/serial.hpp
pendel/trial.o: lib/base.hpp lib/util.hpp lib/stepper.hpp
lib/test/event_queue_test.o: lib/test/mock.hpp lib/util.hpp
lib/test/event_queue_test.o: lib/event_queue.hpp lib/error.hpp
lib/test/rotary_encoder_test.o: lib/test/mock.hpp lib/rotary_encoder.hpp
lib/test/stepper_test.o: lib/test/mock.hpp lib/util.hpp lib/stepper.hpp
lib/test/util_test.o: lib/test/mock.hpp lib/util.hpp
lib/test/run_tests.o: lib/test/util_test.hpp lib/test/mock.hpp lib/util.hpp
lib/test/run_tests.o: lib/test/event_queue_test.hpp lib/event_queue.hpp
lib/test/run_tests.o: lib/error.hpp lib/test/stepper_test.hpp lib/stepper.hpp
lib/test/run_tests.o: lib/test/rotary_encoder_test.hpp lib/rotary_encoder.hpp
lib/test/simulate.o: lib/stepper.hpp
