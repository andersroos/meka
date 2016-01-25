# Most binaries needed here are installed in Ubuntu with apt-get (gcc-avr, avr-libc, avrdude). Arduino IDE
# (arduino.cc) is also needed. To build libs in the IDE, compile in the IDE with verbose compiler output
# (preferences) then find the .a file location in tmp and copy it to lib dir (libcore-arduino-uno.a).

ARDUINO_CORE_DIR = $(ARDUINO_IDE_DIR)/hardware/arduino/avr/cores/arduino

AVR_CXX = avr-g++

AVR_OBJCOPY = avr-objcopy

AVR_SIZE = avr-size

AVR_UPLOAD = avrdude

ARDUINO_INCLUDES = \
	-I$(ARDUINO_CORE_DIR) \
	-I$(ARDUINO_CORE_DIR)/../../variants/standard
#	-I$(ARDUINO_DIR)/libraries/Servo/src \
#	-I$(ARDUINO_DIR)/libraries/Servo/src/avr

ARDUINO_CXXFLAGS  = -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics # Skip not needed/bad stuff.
ARDUINO_CXXFLAGS += -mmcu=atmega328p -DF_CPU=16000000L # Set target format and cpu speed.
ARDUINO_CXXFLAGS += -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR # Arduino IDE compile does this.
ARDUINO_CXXFLAGS += -DARDUINO=10606  # Arduino IDE flag, Arduino-Makefile uses -DARDUINO=166 and says it is the version.
ARDUINO_CXXFLAGS += -D__PROG_TYPES_COMPAT__ # Used by Arduino-Makefile, not sure what for.
ARDUINO_CXXFLAGS += $(ARDUINO_INCLUDES)

ARDUINO_LIBS = -lcore-arduino-uno -lm -lc

ARDUINO_LDFLAGS = -Os -Wl,--gc-sections -mmcu=atmega328p -L$(ARDUINO_IDE_DIR)/lib

ARDUINO_UPLOADFLAGS =  -q -F -V
ARDUINO_UPLOADFLAGS += -C $(ARDUINO_IDE_DIR)/hardware/tools/avr/etc/avrdude.conf
ARDUINO_UPLOADFLAGS += -c arduino -p atmega328p  -b 115200
ARDUINO_UPLOADFLAGS += -P $(PORT)

# Compile.
%.o: %.cpp $(call rwildcard, $(BASE_DIR)/, *.hpp)
	$(AVR_CXX) $(CXXFLAGS) $(ARDUINO_CXXFLAGS) -c -o $@ $<

# Link.
%.elf: %.o $(EXTRA_OBJS)
	$(AVR_CXX) $(ARDUINO_LDFLAGS) -o $@ $^ $(ARDUINO_LIBS)

# Binary conversion.
%.hex: %.elf
	$(AVR_OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $< $(patsubst %.elf,%.eep,$<) # Don't know what eep is good for.
	$(AVR_OBJCOPY) -O ihex -R .eeprom $< $@
	$(AVR_SIZE) --mcu=atmega328p -C --format=avr $< # Display program footprint.

install: $(MAIN).hex
	$(AVR_UPLOAD) $(ARDUINO_UPLOADFLAGS) -U flash:w:$<:i

