INCLUDES=./include
CFLAGS= 
VPATH=src/obj:include

all: lib 
lib: lib/libsense.a

src/obj/%.o: src/*/%.c %.h src/obj
	cc $(CFLAGS) -c -o $@ $< -I$(INCLUDES)

lib/libsense.a: src/obj/framebuffer.o src/obj/i2c.o src/obj/humidity_temp.o src/obj/pressure.o src/obj/joystick.o src/obj/gyro.o src/obj/mag.o src/obj/font.o
	ar rcs $@ $^

clean: lib-clean 
lib-clean: 
	rm -f lib/libsense.a src/obj/*.o
	rmdir src/obj

install: lib/libsense.a ~/lib ~/include
	cp lib/libsense.a ~/lib
	cp include/*.h ~/include

~/include:
	mkdir -p ~/include

~/lib:
	mkdir -p ~/lib

src/obj:
	mkdir -p src/obj

uninstall:
	rm -rf ~/include/sense ~/include/libsense.h
	rm -f ~/lib/libsense.a

.PHONY: all clean lib install uninstall
