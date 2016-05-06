# Parameters for compiling
#
#	 add tap
#	$ brew tap sudar/arduino-mk
#	 to install the last stable release
#
#	$ brew install arduino-mk
#	 to install the development version
#
#	$ brew install --HEAD arduino-mk
#

#Arduino makefile info
ARDUINO_DIR     = /Applications/Development/Arduino.app/Contents/Java
ARDMK_DIR       = /usr/local/opt/arduino-mk
OBJDIR          = ../build
ISP_PROG     	= usbasp
AVRDUDE         = /Users/xcorex/Documents/Arduino/hardware/teenyriot/avr/tools/avrdude/macosx/avrdude

# Board Information
ARCHITECTURE 			= avr
ALTERNATE_CORE			= TeenyRiot
BOARD_TAG			= attinyx4
ARDUINO_PORT			=

CPPFLAGS        += -DARDUINO_AVR_ATTINYX4

# User Library Path
USER_LIB_PATH   = ../libraries
#ARDUINO_PLATFORM_LIB_PATH =
ARDUINO_LIBS = TeenySynth
ARDUINO_QUIET = 1

# Auto-detected from boards.txt
MCU             = attiny84
F_CPU           = 8000000
# AVRDUDE_ARD_PROGRAMMER  = stk500v2
# AVRDUDE_ARD_BAUDRATE    = 115200


# Arduino makefile
include $(ARDMK_DIR)/Arduino.mk
