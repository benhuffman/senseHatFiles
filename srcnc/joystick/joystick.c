#define _GNU_SOURCE
#define EV_NAME "Raspberry Pi Sense HAT Joystick"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include "framebuffer.h"

typedef struct {
	int fd;
	char name[256];
} pi_joystick_t;

/* not really a thing. Just here for compatibility */
static pi_joystick_t joystick;
static pi_joystick_t *result = NULL;
static int _jscount=0;

/*getJoystickDevice
*/
int *jFD=NULL;
int *getJSFD(void);
pi_joystick_t* getJoystickDevice(){
    _jscount++;
	result=&joystick;
    jFD=getJSFD();
	return result;
}

/*freeJoystick
  make things crash if used wrong.
*/
void freeJoystick(pi_joystick_t* device){
    _jscount--;
    if(_jscount>0) return;
    if(_jscount<0) raise(SIGSEGV); // double free
    result = NULL;
}

/*pollJoystick
  processes joystick input passing code to the callback function,
  device: The joystick to poll (ignored)
  callback: a function of the form void fn(int) that is called when an event occurs
  timeout: the timeout in milliseconds for the poll event: 0=do not wait
*/
void pollJoystick(pi_joystick_t* device,
        void (*callback)(unsigned int code),
        int timeout)
{
    struct pollfd evpoll = {
        .events = POLLIN,
        .fd = jFD[0]
    };
    //fprintf(stderr,"polling %d %d\n",jFD[0],jFD[1]);
    if (poll(&evpoll,1,timeout) > 0)
    {
        int c=-1;
        while(read(jFD[0],&c,sizeof(c))==sizeof(c)) {
            if(c != -1) {
                switch(c) {
                    case KEY_UP:
                    case KEY_DOWN:
                    case KEY_LEFT:
                    case KEY_RIGHT:
                    case KEY_ENTER:
		                if(callback)
                        {
			                callback(c);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}
