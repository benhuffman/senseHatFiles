#ifndef __PI_SENSE_FRAMEBUFFER
#define __PI_SENSE_FRAMEBUFFER

#include <stdint.h>
#include <linux/fb.h>

typedef struct {
	uint16_t pixel[8][8];
} sense_fb_bitmap_t;

typedef struct{
	int fd;
	struct fb_fix_screeninfo info;
	sense_fb_bitmap_t* bitmap;
} pi_framebuffer_t;

pi_framebuffer_t* getFBDevice();
void freeFrameBuffer(pi_framebuffer_t* device);
void clearBitmap(sense_fb_bitmap_t* bitmap,uint16_t color);
void useBitmap(pi_framebuffer_t* device, sense_fb_bitmap_t* source);
void setPixel(sense_fb_bitmap_t* bitmap,int x,int y,uint16_t color);
uint16_t getColor(int red,int green,int blue);
#endif
