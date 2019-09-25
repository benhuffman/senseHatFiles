#ifndef __PI_SENSE_JOYSTICK
#define __PI_SENSE_JOYSTICK
typedef struct {
	int fd;
	char name[256];
} pi_joystick_t;

pi_joystick_t* getJoystickDevice();
void freeJoystick(pi_joystick_t* device);
void pollJoystick(pi_joystick_t* device, void (*callback)(unsigned int code),int timeout);
#endif
