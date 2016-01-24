
MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BASE_DIR := $(realpath $(CURDIR)/$(MAKEFILE_DIR))

ARDUINO_DIR = /home/ygram/opt/arduino-1.6.6
ARDUINO_CORE_DIR = $(ARDUINO_DIR)/hardware/arduino/avr/cores/arduino
ARDUINO_ARM_DIR = $(ARDUINO_DIR)/hardware/tools/arm/bin

ARDUINO_LIBS = -lcore-teensy -lm -lc -larm_cortexM4l_math

ARM_CXX = $(ARDUINO_ARM_DIR)/arm-none-eabi-g++
ARM_OBJCOPY = $(ARDUINO_ARM_DIR)/arm-none-eabi-objcopy

UPLOAD = /home/ygram/src/Opensource/teensy_loader_cli/teensy_loader_cli

UPLOADFLAGS = -w -mmcu=mk20dx256

CXXFLAGS  =  -c -g -std=c++11 -Wall
CXXFLAGS += -Os # Optimize for size.

TEENSY_CXXFLAGS  = -D__MK20DX256__ # ?
TEENSY_CXXFLAGS += -mcpu=cortex-m4 -mthumb # Compile for Cortex M4 and Teensy.
TEENSY_CXXFLAGS += -fno-exceptions  -fno-rtti -felide-constructors # Skip not needed/bad stuff.
TEENSY_CXXFLAGS += -fsingle-precision-constant # Constants are floats not double.
TEENSY_CXXFLAGS += -DTEENSYDUINO=127 -DARDUINO=10606 # Teensy/Arduino IDE version?
TEENSY_CXXFLAGS += -DF_CPU=96000000 # Clock frequency.
TEENSY_CXXFLAGS += -DARDUINO_ARCH_AVR -DUSB_SERIAL # ?
TEENSY_CXXFLAGS += -DLAYOUT_US_ENGLISH # ?
TEENSY_CXXFLAGS += -I$(ARDUINO_DIR)/hardware/teensy/avr/cores/teensy3

TEENSY_LDFLAGS  = -Os -Wl,--gc-sections,--relax,--defsym=__rtc_localtime=1453635298
TEENSY_LDFLAGS += -mcpu=cortex-m4 -mthumb -L$(ARDUINO_DIR)/lib
TEENSY_LDFLAGS += -T/home/ygram/opt/arduino-1.6.6/hardware/teensy/avr/cores/teensy3/mk20dx256.ld

MAIN = blink
EXTRA_OBJS = 

# Compile.
%.o: %.cpp $(call rwildcard, $(BASE_DIR)/, *.hpp)
	$(ARM_CXX) $(CXXFLAGS) $(TEENSY_CXXFLAGS) -c -o $@ $<

# Link.
%.elf: %.o $(EXTRA_OBJS)
	$(ARM_CXX) $(TEENSY_LDFLAGS) -o $@ $^ $(ARDUINO_LIBS)

# Binary conversion.
%.hex: %.elf
	$(ARM_OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $< $(patsubst %.elf,%.eep,$<) # Don't know what eep is good for.
	$(ARM_OBJCOPY) -O ihex -R .eeprom $< $@

default: build

build: $(MAIN).hex

install: $(MAIN).hex
	$(UPLOAD) $(UPLOADFLAGS) $<

PORT = /dev/ttyACM0
console:
	until $$(stty -F $(PORT) cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts); do sleep 0.2; done
	cat $(PORT)

