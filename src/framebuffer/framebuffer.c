#define _GNU_SOURCE
#define FB_NAME "RPi-Sense FB"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <dirent.h>
#include "framebuffer.h"

/*isFrameBuffer
  dev: a directory entry for the device file to be checked
  returns: true if device is a framebuffer
*/
static int isFrameBuffer(const struct dirent *dev){
	return strncmp("fb",dev->d_name,2)==0;
}

/*getFrameBuffer
  name: the string name of the framebuffer to retrieve
  returns: a valid framebuffer structure with the bitmap field memory mapped to the framebuffer, or null on failure
  Note: function allocates a pi_framebuffer_t object on success which must be freed with a call to freeFrameBuffer()
*/
static pi_framebuffer_t* getFBDevice(const char* name){
	pi_framebuffer_t* result=0;
	int fd=open(name,O_RDWR);
	if (fd<0) return 0;

	result=malloc(sizeof(pi_framebuffer_t));
	ioctl(fd,FBIOGET_FSCREENINFO,&result->info);
	if (strcmp(FB_NAME,result->info.id)!=0){
		close(fd);
		free(result);
		return 0;
	}
	result->fd=fd;
	result->bitmap=mmap(0,128,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	return result;
}

/*getFBDevice
  returns a pi_framebuffer_t object describing the sense hat frame buffer, or null on failure
  Note: function allocates a pi_framebuffer_t object on success which must be freed with a call to freeFrameBuffer()
*/
pi_framebuffer_t* getFrameBuffer(){
	pi_framebuffer_t* result=0;
	struct dirent **list;
	int i,ndev;
	ndev=scandir("/dev",&list,isFrameBuffer,versionsort);
	if(ndev<=0) return 0; /* No framebuffers found */
	for (i=0;i<ndev;i++){
		char fname[64];
		snprintf(fname,sizeof(fname),"/dev/%s",list[i]->d_name);
		result=getFBDevice(fname);
		if (result) break;
	}
	for (i=0;i<ndev;i++)
		free(list[i]);
	return result;
}

/*freeFrameBuffer
  frees and unmaps a previously allocated frame buffer
*/
void freeFrameBuffer(pi_framebuffer_t* device){
	munmap(device->bitmap,sizeof(device->bitmap));
	close(device->fd);
	free(device);
}
/*setPixel
  bitmap: a bitmap object to modify
  x,y: the 0 based coordinates 0-7 of the pixel to modify
  color: the color to change
  Modifies the color of the pixel at x,y
*/
void setPixel(sense_fb_bitmap_t* bitmap,int x,int y,uint16_t color){
	bitmap->pixel[y][x]=color;
}

/*clearFrameBuffer
  fb: a sense hat frame buffer object
  color: the fill color for the bitmap
  Fills the bitmap with the color
*/
void clearFrameBuffer(pi_framebuffer_t* fb,uint16_t color){
	int i,j;
	for (i=0;i<8;i++)
		for (j=0;j<8;j++)
			fb->bitmap->pixel[i][j]=color;
}
/*useBitmap
  device: the sense hat framebuffer device
  source: A bitmap to use to overwrite the sense hat framebuffer
  draws the bitmap on the sense hat framebuffer
*/
void useBitmap(pi_framebuffer_t* device, sense_fb_bitmap_t* source){
	int i,j;
	for (i=0;i<8;i++)
		for (j=0;j<8;j++)
			device->bitmap->pixel[i][j]=source->pixel[i][j];
}

/*getColor
  red: the 8 bit red component
  green: the 8 bit green component
  blue: the 8 bit blue component
  returns a 16 bit representation of the 32 bit color specified by the arguments
*/
uint16_t getColor(int red,int green,int blue){
	red=(float)red / 255.0 * 31.0 + 0.5;
	green=(float)green/ 255.0 * 63.0 + 0.5;
	blue=(float)blue / 255.0 * 31.0 + 0.5;
	return red<<11|green<<5|blue;
}
