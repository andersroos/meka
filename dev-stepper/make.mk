
MAIN = dev-stepper/stepper
# MAIN = dev-stepper/simple-move
# MAIN = dev-stepper/speed-trial
# MAIN = dev-stepper/changing-speed-trial

BOARD = Teensy32
# BOARD = ArduinoUno

BAUD_RATE = 9600

dev-stepper/simulate.o: dev-stepper/simulate.cpp
	$(TEST_CXX) $(CXXFLAGS) $(TEST_CXXFLAGS) -c -o $@ $<

simulate: dev-stepper/simulate.o
	$(TEST_CXX) -std=c++11 -o dev-stepper/simulate $<
