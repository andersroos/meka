# Arduino IDE (arduino.cc), Teensyduino (https://www.pjrc.com/teensy/td_download.html) and Teensy Loader Cli
# (https://github.com/PaulStoffregen/teensy_loader_cil) are needed. Compile the lib in the Arduino IDE then
# copy to (libcore-teensy32.a).

ARM_DIR = $(ARDUINO_IDE_DIR)/hardware/tools/arm/bin

ARM_CXX = $(ARM_DIR)/arm-none-eabi-g++

TEENSY_LIBS = -lcore-teensy32 -lm -lc -larm_cortexM4l_math

ARM_OBJCOPY = $(ARM_DIR)/arm-none-eabi-objcopy

UPLOAD = $(HOME)/src/Opensource/teensy_loader_cli/teensy_loader_cli

UPLOADFLAGS = -w -mmcu=mk20dx256

TEENSY_CXXFLAGS  = -D__MK20DX256__ # ?
TEENSY_CXXFLAGS += -mcpu=cortex-m4 -mthumb # Compile for Cortex M4 and Teensy.
TEENSY_CXXFLAGS += -fno-exceptions  -fno-rtti -felide-constructors # Skip not needed/bad stuff.
TEENSY_CXXFLAGS += -fsingle-precision-constant # Constants are floats not double.
TEENSY_CXXFLAGS += -fgnu-keywords # Allow typeof (which is not c++ 11).
TEENSY_CXXFLAGS += -DTEENSYDUINO=127 -DARDUINO=10606 # Teensy/Arduino IDE version?
TEENSY_CXXFLAGS += -DF_CPU=96000000 # Clock frequency.
TEENSY_CXXFLAGS += -DARDUINO_ARCH_AVR -DUSB_SERIAL # ?
TEENSY_CXXFLAGS += -DLAYOUT_US_ENGLISH # ?
TEENSY_CXXFLAGS += -I$(ARDUINO_IDE_DIR)/hardware/teensy/avr/cores/teensy3

TEENSY_LDFLAGS  = -Os -Wl,--gc-sections,--relax,--defsym=__rtc_localtime=1453635298
TEENSY_LDFLAGS += -mcpu=cortex-m4 -mthumb -L$(ARDUINO_IDE_DIR)/lib
TEENSY_LDFLAGS += -T$(ARDUINO_IDE_DIR)/hardware/teensy/avr/cores/teensy3/mk20dx256.ld

# Compile.
%.o: %.cpp
	$(ARM_CXX) $(CXXFLAGS) $(TEENSY_CXXFLAGS) -c -o $@ $<

# Link.
%.elf: %.o $(EXTRA_OBJS)
	$(ARM_CXX) $(TEENSY_LDFLAGS) -o $@ $^ $(TEENSY_LIBS)

# Binary conversion.
%.hex: %.elf
	$(ARM_OBJCOPY) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 $< $(patsubst %.elf,%.eep,$<) # Don't know what eep is good for.
	$(ARM_OBJCOPY) -O ihex -R .eeprom $< $@

install: $(MAIN).hex
	$(UPLOAD) $(UPLOADFLAGS) $<
