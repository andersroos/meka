MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BASE_DIR := $(realpath $(CURDIR)/$(MAKEFILE_DIR))

ARDUINO_IDE_DIR ?= $(HOME)/opt/arduino-1.6.6

PORT ?= /dev/ttyACM0

#
# Common variables and settings.
#

CXXFLAGS = -c -g -std=c++11 -Wall -Os

BAUD_RATE = 9600

#BOARD = ArduinoUno
BOARD = Teensy32

MAIN = pendel
#MAIN = pendel-trial
#MAIN = stepper-changing-speed-trial
#MAIN = stepper-speed-trial
#MAIN = pendel-speed-trial
#MAIN = stepper

EXTRA_OBJS = # $(ARDUINO_DIR)/libraries/Servo/src/avr/Servo.o

default: build

#
# Test/simulation.
#

TEST_CXX = g++

TEST_OBJS = test/run-tests.o

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

test/run-tests.o: test/event-test.hpp test/mock.hpp lib/util.hpp
test/run-tests.o: lib/event.hpp lib/error.hpp test/stepper-test.hpp
test/run-tests.o: lib/stepper.hpp
test/simulate.o: test/mock.hpp lib/stepper.hpp
test/event-test.o: test/mock.hpp lib/util.hpp lib/event.hpp lib/error.hpp
test/stepper-test.o: test/mock.hpp lib/util.hpp lib/stepper.hpp
lib/event.o: lib/error.hpp
pendel-speed-trial.o: pendel-pins.hpp
pendel-trial.o: lib/base.hpp lib/util.hpp lib/stepper.hpp pendel-pins.hpp
pendel.o: lib/base.hpp lib/util.hpp lib/stepper.hpp lib/event.hpp
pendel.o: lib/error.hpp pendel-pins.hpp
stepper-changing-speed-trial.o: lib/base.hpp lib/util.hpp lib/stepper.hpp
stepper-changing-speed-trial.o: pendel-pins.hpp
stepper-speed-trial.o: lib/base.hpp lib/stepper.hpp pendel-pins.hpp
