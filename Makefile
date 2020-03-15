INCLUDES := ./include
CFLAGS := 
SRC := src
SRCNC := srcnc
VPATH := $(SRC)/obj:include
OBJDIR := $(SRC)/obj

all: library
library: lib/libsense.a lib/libsensenc.a

$(SRC)/obj/%.o: $(SRC)/*/%.c %.h | $(SRC)/obj
	cc $(CFLAGS) -c -o $@ $< -I$(INCLUDES)
$(SRCNC)/obj/%.o: $(SRCNC)/*/%.c %.h | $(SRCNC)/obj
	cc $(CFLAGS) -c -o $@ $< -I$(INCLUDES)


lib/libsense.a: $(SRC)/obj/framebuffer.o $(SRC)/obj/i2c.o $(SRC)/obj/humidity_temp.o $(SRC)/obj/pressure.o $(SRC)/obj/joystick.o $(SRC)/obj/gyro.o $(SRC)/obj/mag.o $(SRC)/obj/font.o | lib
	ar rcs $@ $(SRC)/obj/*.o
lib/libsensenc.a: $(SRCNC)/obj/framebuffer.o $(SRCNC)/obj/i2c.o $(SRCNC)/obj/humidity_temp.o $(SRCNC)/obj/pressure.o $(SRCNC)/obj/joystick.o $(SRCNC)/obj/gyro.o $(SRCNC)/obj/mag.o $(SRCNC)/obj/font.o | lib
	ar rcs $@ $(SRCNC)/obj/*.o

clean: lib-clean 
lib-clean: 
	rm -rf src/obj ncsrc/obj lib

install: lib/libsense.a lib/libsensenc.a | ~/lib ~/include
	cp lib/libsense.a lib/libsensenc.a ~/lib
	cp include/*.h ~/include

# Rule to create directories as needed
~/include ~/lib ncsrc/obj src/obj lib :
	mkdir -p $@

uninstall:
	rm -rf ~/include/sense ~/include/libsense.h
	rm -f ~/lib/libsense.a

.PHONY: all clean lib install uninstall
