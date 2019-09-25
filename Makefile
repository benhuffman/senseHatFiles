INCLUDES=./include
CFLAGS= 
VPATH=src/obj:include

lib: lib/libsense.a

src/obj/%.o: src/*/%.c %.h
	cc $(CFLAGS) -c -o $@ $< -I$(INCLUDES)

lib/libsense.a: src/obj/framebuffer.o src/obj/i2c.o src/obj/humidity_temp.o src/obj/pressure.o src/obj/joystick.o src/obj/gyro.o src/obj/mag.o src/obj/font.o
	ar rcs $@ $^

lib-clean: 
	rm -f lib/libsense.a src/obj/*.o

install: lib/libsense.a ~/lib ~/include
	cp lib/libsense.a ~/lib
	cp include/*.h ~/include

~/include:
	mkdir -p ~/include

~/lib:
	mkdir ~/lib

~/bin:
	mkdir ~/bin

uninstall:
	rm -rf ~/include/sense ~/include/libsense.h
	rm -f ~/lib/libsense.a

all: lib 

clean: lib-clean 
