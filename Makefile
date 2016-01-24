
MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BASE_DIR := $(realpath $(CURDIR)/$(MAKEFILE_DIR))

ARDUINO_DIR = /home/ygram/opt/arduino-1.6.6
ARDUINO_CORE_DIR = $(ARDUINO_DIR)/hardware/arduino/avr/cores/arduino

PORT = /dev/ttyACM0
BAUD_RATE = 9600
MAIN = stepper-speed-trial

AVR_CXX = avr-g++

CXX = g++

OBJS = $(MAIN).o # $(ARDUINO_DIR)/libraries/Servo/src/avr/Servo.o

OBJCOPY = avr-objcopy
SIZE = avr-size

ARDUINO_INCLUDES = \
	-I$(ARDUINO_CORE_DIR) \
	-I$(ARDUINO_CORE_DIR)/../../variants/standard \
	-I$(ARDUINO_DIR)/libraries/Servo/src \
	-I$(ARDUINO_DIR)/libraries/Servo/src/avr

TEST_OBJS = test/run-tests.o \
	    test/stepper-test.o \

# To build core.a build something in the IDE with verbose compiler output (preferences) then find the .a
# file location in tmp and copy it to lib dir (libcore-arduino-uno.a).
ARDUINO_LIBS = -lcore-arduino-uno -lm -lc

CXXFLAGS  =  -c -g -std=c++11 -Wall
CXXFLAGS += -Os # Optimize for size.

ARDUINO_CXXFLAGS  = -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics # Skip not needed/bad stuff.
ARDUINO_CXXFLAGS += -mmcu=atmega328p -DF_CPU=16000000L # Set target format and cpu speed.
ARDUINO_CXXFLAGS += -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR # Arduino IDE compile does this.
ARDUINO_CXXFLAGS += -DARDUINO=10606  # Arduino IDE flag, Arduino-Makefile uses -DARDUINO=166 and says it is the version.
ARDUINO_CXXFLAGS += -D__PROG_TYPES_COMPAT__ # Used by Arduino-Makefile, not sure what for.
ARDUINO_CXXFLAGS += $(ARDUINO_INCLUDES)

TEST_CXXFLAGS  = -I.

TEST_LIBS = -lboost_unit_test_framework

LDFLAGS = -Os -Wl,--gc-sections -mmcu=atmega328p -L$(ARDUINO_DIR)/lib

UPLOADFLAGS  = -q -F -V
UPLOADFLAGS += -C $(ARDUINO_DIR)/hardware/tools/avr/etc/avrdude.conf
UPLOADFLAGS += -c arduino -p atmega328p  -b 115200
UPLOADFLAGS += -P $(PORT)

# Test compile.
test/%.o: test/%.cpp $(call rwildcard, $(BASE_DIR)/, *.hpp)
	$(CXX) $(CXXFLAGS) $(TEST_CXXFLAGS) -c -o $@ $<

# Compile.
%.o: %.cpp $(call rwildcard, $(BASE_DIR)/, *.hpp)
	$(AVR_CXX) $(CXXFLAGS) $(ARDUINO_CXXFLAGS) -c -o $@ $<

# Link.
%.elf: $(OBJS)
	$(AVR_CXX) $(LDFLAGS) -o $@ $^ $(ARDUINO_LIBS)

# Binary conversion.
%.hex: %.elf
	$(OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $< $(patsubst %.elf,%.eep,$<) # Don't know what eep is good for.
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) --mcu=atmega328p -C --format=avr $< # Display program footprint.

default: build

build: $(MAIN).hex

install: $(MAIN).hex
	avrdude $(UPLOADFLAGS) -U flash:w:$<:i

console:
	until $$(stty -F $(PORT) cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts); do sleep 0.2; done
	cat $(PORT)

test: $(TEST_OBJS)
	$(CXX) -std=c++11 -o ./run-tests $(TEST_OBJS) $(TEST_LIBS)
	./run-tests

simulate: test/simulate.o
	$(CXX) -std=c++11 -o $@ $<

clean:
	\rm -f *.o test/*.o *.elf *.hex *~ *.eef *.eep run-tests simulate

depend:
	makedepend -Y test/*.cpp test/*.hpp lib/*.hpp *.cpp

.PRECIOUS: %.o %.elf
# DO NOT DELETE

test/simulate.o: test/mock.hpp lib/stepper.hpp
test/stepper-test.o: test/mock.hpp lib/stepper.hpp
stepper-speed-trial.o: lib/base.hpp lib/stepper.hpp
