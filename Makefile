INCLUDES := ./include
CFLAGS := 
VPATH = $(SRC)/obj:include
OBJDIR = $(SRC)/obj
#FB != ls /dev/fb* | wc -w
#ifndef SRC
#ifeq "$(FB)" "1"
#SRC := srcnc
#else
SRC := src
#endif
#endif

all: library
library: lib/libsense.a

$(SRC)/obj/%.o: $(SRC)/*/%.c %.h | $(SRC)/obj
	cc $(CFLAGS) -c -o $@ $< -I$(INCLUDES)

lib/libsense.a: $(SRC)/obj/framebuffer.o $(SRC)/obj/i2c.o $(SRC)/obj/humidity_temp.o $(SRC)/obj/pressure.o $(SRC)/obj/joystick.o $(SRC)/obj/gyro.o $(SRC)/obj/mag.o $(SRC)/obj/font.o | lib
	ar rcs $@ $(SRC)/obj/*.o

clean: lib-clean 
lib-clean: 
	rm -rf $(SRC)/obj lib

install: lib/libsense.a | ~/lib ~/include
	cp lib/libsense.a ~/lib
	cp include/*.h ~/include

# Rule to create directories as needed
~/include ~/lib $(SRC)/obj lib :
	mkdir -p $@

uninstall:
	rm -rf ~/include/sense ~/include/libsense.h
	rm -f ~/lib/libsense.a

.PHONY: all clean lib install uninstall
.DEFAULT_GOAL := all
