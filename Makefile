INCLUDES := ./include
CFLAGS := 
VPATH := src/obj:include
OBJDIR := src/obj

all: library
library: lib/libsense.a 

src/obj/%.o: src/*/%.c %.h | src/obj
	cc $(CFLAGS) -c -o $@ $< -I$(INCLUDES)

lib/libsense.a: src/obj/framebuffer.o src/obj/i2c.o src/obj/humidity_temp.o src/obj/pressure.o src/obj/joystick.o src/obj/gyro.o src/obj/mag.o src/obj/font.o | lib
	ar rcs $@ src/obj/*.o

clean: lib-clean 
lib-clean: 
	rm -rf src/obj lib

install: lib/libsense.a | ~/lib ~/include
	cp lib/libsense.a ~/lib
	cp include/*.h ~/include

# Rule to create directories as needed
~/include ~/lib src/obj lib :
	mkdir -p $@

uninstall:
	rm -rf ~/include/sense ~/include/libsense.h
	rm -f ~/lib/libsense.a

.PHONY: all clean lib install uninstall
