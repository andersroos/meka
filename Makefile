
MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BASE_DIR := $(realpath $(CURDIR)/$(MAKEFILE_DIR))

PORT = /dev/ttyACM0
BAUD_RATE = 9600
MAIN = stepper

CXX = avr-g++

OBJCOPY = avr-objcopy
SIZE = avr-size

ARDUINO_DIR = /home/ygram/opt/arduino-1.6.6

ARDUINO_CORE_DIR = $(ARDUINO_DIR)/hardware/arduino/avr/cores/arduino

ARDUINO_INCLUDES = \
	-I$(ARDUINO_CORE_DIR) \
	-I$(ARDUINO_CORE_DIR)/../../variants/standard

# To build core.a build something in the IDE with verbose compiler output (preferences) then find the .a
# file location in tmp and copy it to lib dir (libcore.a).
ARDUINO_LIBS = -lcore -lm -lc

CXXFLAGS  =  -c -g -std=gnu++11 -Wall
CXXFLAGS += -Os # Optimize for size.
CXXFLAGS += -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics # Skip not needed/bad stuff.
CXXFLAGS += -mmcu=atmega328p -DF_CPU=16000000L # Set target format and cpu speed.
CXXFLAGS += -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR # Arduino IDE compile does this.
CXXFLAGS += -DARDUINO=10606  # Arduino IDE flag, Arduino-Makefile uses -DARDUINO=166 and says it is the version.
CXXFLAGS += -D__PROG_TYPES_COMPAT__ # Used by Arduino-Makefile, not sure what for.
CXXFLAGS += $(ARDUINO_INCLUDES)

LDFLAGS = -Os -Wl,--gc-sections -mmcu=atmega328p -L$(ARDUINO_DIR)/lib

UPLOADFLAGS  = -q -F -V
UPLOADFLAGS += -C $(ARDUINO_DIR)/hardware/tools/avr/etc/avrdude.conf
UPLOADFLAGS += -c arduino -p atmega328p  -b 115200
UPLOADFLAGS += -P $(PORT)

# Compile.
%.o: %.cpp $(call rwildcard, $(BASE_DIR)/, *.hpp)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Link.
%.elf: %.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(ARDUINO_LIBS)

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

clean:
	\rm -f *.o *.elf *.hex *~ *.eef *.eep

.PRECIOUS: %.o %.elf
