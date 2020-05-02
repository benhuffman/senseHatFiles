#define _GNU_SOURCE
#define FB_NAME "RPi-Sense FB"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "framebuffer.h"
#include <ncurses.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>

static pi_framebuffer_t cscreen;
static pi_framebuffer_t* result=NULL;
static pi_framebuffer_t* allocated=NULL;
static sense_fb_bitmap_t *cbitmap;
static sense_fb_bitmap_t obitmap;
static int volatile marker=0;
static pid_t volatile child=1;
static int fbcount=0;
int joystickFD[2]={-1,-1};
int *getJSFD(void) {
    return joystickFD;
}

void handle_sigchld(int sig) {
    int status;
    while (waitpid((pid_t)(-1), &status, WNOHANG) > 0) {
        if(WIFSIGNALED(status) && WTERMSIG(status)==SIGSEGV)
            kill(0,SIGSEGV);
    }
    child=0;
}

/* tries to find the closest color in the existing terminal set
 * to the one we're trying to show on the hat. Doesn't even
 * pretend to use color theory */

/* L2 distance of 16-bit colors */
int colorDistance(short term, uint16_t hat) {
    short tr,tg,tb;
    /* green got an extra bit that we want to ignore, I think */
    /* color_content is scaled to 1000 for some reason */
    unsigned short hr=1000*((hat&65535)>>11)/32,
                   hg=1000*((hat&2047)>>6)/32,
                   hb=1000*(hat&31)/32;
    color_content(term,&tr,&tg,&tb);
    int dist= (tr-hr)*(tr-hr)+(tg-hg)*(tg-hg)+(tb-hb)*(tb-hb);
    //fprintf(stderr,"(%d,%d,%d)-(%d,%d,%d)=%d\n",hr,hg,hb,tr,tg,tb,dist);
    return dist;
}
int closestColor(uint16_t hatColor) {
    int cc=0, dmin=colorDistance(0,hatColor);

    //fprintf(stderr,"%d %d\n",0,dmin);
    for(int c=1; c<8; c++) {
        int dc=colorDistance(c,hatColor);
        //fprintf(stderr,"%d %d\n",c,dc);
        if(dc<dmin) {
            cc=c;
            dmin=dc;
        }
    }
    return cc;
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
                int color=closestColor(cbitmap->pixel[7-x][y]);
                attron(COLOR_PAIR(color));
                addch('#');
                //addch('0'+(char)color);
                attroff(COLOR_PAIR(color));
            }else{
                addch(' ');
            }
        }
        addch('|');
    }
    mvprintw(16,0,"+-+-+-+-+-+-+-+-+");
    mvprintw(17,0," USB PORTS HERE");
    mvprintw(18,0,"");
    //mvprintw(17,0,"%d\t%lu",marker++,child);
    refresh();
}

/*getFrameBuffer
  returns a pi_framebuffer_t object describing the sense hat frame buffer, or null on failure
  Note: function allocates a pi_framebuffer_t object on success which must be freed with a call to freeFrameBuffer()
*/
pi_framebuffer_t* getFrameBuffer(){
    //fprintf(stderr,"getFB: fbcount: %d\n",fbcount);
    fbcount++;
    if(allocated)
    {
        if(mprotect(result->bitmap,
                sizeof(sense_fb_bitmap_t),
                PROT_READ|PROT_WRITE)
          ) fprintf(stderr,"failed to mprotect unlock\n");
        return result;
    }
	int i,ndev;
    cbitmap=mmap(NULL, sizeof(sense_fb_bitmap_t),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS,
            -1, 0);
    pipe2(joystickFD,O_NONBLOCK);
    initscr();
    start_color();
    for(int i=1; i<8; i++)
        init_pair(i,i,i);
    noecho();
    cbreak();
    keypad(stdscr,TRUE);
    nodelay(stdscr,TRUE);
	result=&cscreen;
    result->bitmap=cbitmap;
    for(int i=0; i<8; i++) {
        for(int j=0;j<8;j++) {
            cbitmap->pixel[i][j]=0;
            obitmap.pixel[i][j]=1;
        }
    }
    allocated=result;

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
        int c;
        while((c=getch())!=ERR) {
            switch(c) {
                // input_event_codes.h and ncurses.h differ in their codes
                // Also, want to match (arbitrary) pi orientation
                case KEY_UP:
                    c=105; // RIGHT 105
                    break;
                case KEY_DOWN:
                    c=106; // LEFT 106
                    break;
                case KEY_LEFT:
                    c=108; // DOWN 108
                    break;
                case KEY_RIGHT:
                    c=103; // UP 103
                    break;
                case KEY_ENTER:
                case '\n':
                case '\r':
                    c=28;
                    break;
            }
            write(joystickFD[1],&c,sizeof(c));
        }
        drawCFB();
        select(0,NULL,NULL,NULL,&tv);
    }
    tv.tv_sec=1;
    select(0,NULL,NULL,NULL,&tv);
    echo();
    endwin();
    exit(0);
	return result;
}

/*freeFrameBuffer
  Blocks the shared memory from writes (to simulate actually freeing)
  and inserts a pause so the last-drawn image can be seen).
*/
void freeFrameBuffer(pi_framebuffer_t* device){

    //fprintf(stderr,"free; fbcount: %d\n",fbcount);
    if(!fbcount) {
        // Double free.
        raise(SIGSEGV);
        return;
    }
    fbcount--;
    if(fbcount) return;

    allocated=NULL;
    mprotect(result->bitmap,
             sizeof(sense_fb_bitmap_t),
             PROT_NONE);
    //result->bitmap->pixel[0][0]=1;
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
