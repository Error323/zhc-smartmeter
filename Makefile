BOARD_TAG = uno
MONITOR_PORT  = /dev/ttyACM2
ARDUINO_LIBS = MsTimer2 SoftwareSerial Regexp
USER_LIB_PATH = /home/fhuizing/Workspace/Arduino/libraries
ARDUINO_DIR = /home/fhuizing/Workspace/Arduino/arduino-1.6.13
ARCHITECTURE = avr
MONITOR_BAUDRATE = 9600
CXXFLAGS_STD = -std=c++11 -Wall -Wstrict-prototypes -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -fdata-sections -ffunction-sections

include /home/fhuizing/Workspace/Arduino/Arduino-Makefile/Arduino.mk
