#define _GNU_SOURCE
#define EV_NAME "Raspberry Pi Sense HAT Joystick"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <poll.h>
#include <linux/input.h>

typedef struct {
	int fd;
	char name[256];
} pi_joystick_t;

/*isEventDevice
  dev: a directory entry for the device file to be checked
  returns: true if device is an event device
*/
static int isEventDevice(const struct dirent *dir)
{
    return strncmp("event", dir->d_name,
               strlen("event")-1) == 0;
}

/*getEventDevice
  name: the string name of the device to retrieve
  returns: a joystick object or null on failure
  Note: function allocates a pi_joystick_t object on success which must be freed with a call to freeJoystick()
*/
static pi_joystick_t* getEventDevice(const char* name){
	pi_joystick_t* result=0;
	int fd=open(name,O_RDONLY);
	if (fd<0) return 0;

	result=malloc(sizeof(pi_joystick_t));
	ioctl(fd,EVIOCGNAME(sizeof(result->name)),result->name);
	if (strcmp(EV_NAME,result->name)!=0){
		close(fd);
		free(result);
		return 0;
	}
	result->fd=fd;
	return result;
}

/*getJoystickDevice
  returns a pi_joystick_t object describing the sense hat joystick, or null on failure
  Note: function allocates a pi_joystick_t object on success which must be freed with a call to freeJoystick()
*/
pi_joystick_t* getJoystickDevice(){
	pi_joystick_t* result=0;
	struct dirent **list;
	int i,ndev;
	ndev=scandir("/dev/input",&list,isEventDevice,versionsort);
	if(ndev<=0) return 0; /* No event devices found */
	for (i=0;i<ndev;i++){
		char fname[64];
		snprintf(fname,sizeof(fname),"/dev/input/%s",list[i]->d_name);
		result=getEventDevice(fname);
		if (result) break;
	}
	for (i=0;i<ndev;i++)
		free(list[i]);
	return result;
}

/*freeJoystick
  frees and unmaps a previously allocated joystick
*/
void freeJoystick(pi_joystick_t* device){
	if (device){
		close(device->fd);
		free(device);
	}
}

static void processEvents(int evfd,void (*callback)(unsigned int code)){
	struct input_event ev[64];
	int i,rd;
	rd=read(evfd,ev,sizeof(struct input_event)*64);
	if (rd<(int)sizeof(struct input_event))
		return;
	for (i=0;i<rd/sizeof(struct input_event);i+=2){
		if (ev->type !=EV_KEY || ev->value !=1)
			continue;
		if(callback)
			callback(ev->code);
	}
}

/*pollJoystick
  processes joystick input passing code from input.h to the callback function,
  device: The joystick to poll
  callback: a function of the form void fn(int) that is called when an event occurs
  timeout: the timeout in miliseconds for the poll event: 0=do not wait
*/
void pollJoystick(pi_joystick_t* device, void (*callback)(unsigned int code),int timeout){
	struct pollfd evpoll = {
        .events = POLLIN,
		.fd = device->fd
    };
	if (poll(&evpoll,1,timeout) > 0)
		processEvents(evpoll.fd,callback);
}
