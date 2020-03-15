#define _GNU_SOURCE
#define FB_NAME "RPi-Sense FB"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "framebuffer.h"
#include <ncurses.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>

static pi_framebuffer_t cscreen;
static sense_fb_bitmap_t *cbitmap;
static sense_fb_bitmap_t obitmap;
static int volatile marker=0;
static pid_t volatile child=0;

void handle_sigchld(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
  child=0;
}

void drawCFB(void) {
    if(!child) return;
    int diff=0;
    for(int y=0; y<8; y++) {
        for(int x=0;x<8;x++) {
            if(cbitmap->pixel[y][x] != obitmap.pixel[y][x]) {
                diff=1;
                break;
            }
        }
    }
    if(!diff) return;
    for(int y=0; y<8; y++) {
        mvprintw(y*2,0,"+-+-+-+-+-+-+-+-+");
        move(y*2+1,0);
        for(int x=0; x<8; x++) {
            addch('|');
            obitmap.pixel[7-x][y]=cbitmap->pixel[7-x][y];
            if(cbitmap->pixel[7-x][y]) {
                addch('#');
            }else{
                addch(' ');
            }
        }
        addch('|');
    }
    mvprintw(16,0,"+-+-+-+-+-+-+-+-+");
    //mvprintw(17,0,"%d\t%lu",marker++,child);
    refresh();
}

/*getFrameBuffer
  returns a pi_framebuffer_t object describing the sense hat frame buffer, or null on failure
  Note: function allocates a pi_framebuffer_t object on success which must be freed with a call to freeFrameBuffer()
*/
pi_framebuffer_t* getFrameBuffer(){
	pi_framebuffer_t* result=0;
	int i,ndev;
    cbitmap=mmap(NULL, sizeof(sense_fb_bitmap_t),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS,
            -1, 0);
    initscr();
    noecho();
    cbreak();
    keypad(stdscr,TRUE);
	result=&cscreen;
    result->bitmap=cbitmap;
    for(int i=0; i<8; i++) {
        for(int j=0;j<8;j++) {
            cbitmap->pixel[i][j]=0;
            obitmap.pixel[i][j]=1;
        }
    }

    child=fork();
    if(!child) {
        // The child will be the one which actually continues
        // with program logic
        return result;
    }

    struct sigaction sa;
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        perror(0);
        exit(1);
    }

    struct timeval tv;
    tv.tv_sec=0;
    tv.tv_usec=100000;
    while(child){
        drawCFB();
        select(0,NULL,NULL,NULL,&tv);
    }
    //printf("Marker got to: %d\n",marker);
    tv.tv_sec=1;
    select(0,NULL,NULL,NULL,&tv);
    echo();
    endwin();
    exit(0);
	return result;
}

/*freeFrameBuffer
  Essentially a no-op in this case
*/
void freeFrameBuffer(pi_framebuffer_t* device){
    struct timeval tv;
    tv.tv_sec=0;
    tv.tv_usec=200000;
    select(0,NULL,NULL,NULL,&tv);
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
