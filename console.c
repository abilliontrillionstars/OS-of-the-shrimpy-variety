#include "utils.h"
#include "console.h"
#include "serial.h"

#include "kprintf.h"

#define SCREEN_HEIGHT 600
#define SCREEN_WIDTH 800

/*
static u16 fgColor;
static u16 bgColor;
static u16 dimColors[8] = 
{
    0, 0, 0, 0, 0, 0, 0, 0 
    // fill in with color numbers fro the slides
};
static u16 brightColors[8] = 
{
    0, 0, 0, 0, 0, 0, 0, 0 
    // same here
};
*/
enum inputState
{
    NORMAL_CHARS, //initial state
    GOT_ESC,
    GOT_LBRACKET,
    GOT_3,  //GOT_y must be adjacent to GOT_yx
    GOT_3x,
    GOT_4,
    GOT_4x,
    GOT_9,
    GOT_9x,
    GOT_1,
    GOT_1x,
    GOT_1xx
};
enum inputState currentState = NORMAL_CHARS;

void console_init(struct MultibootInfo* mbi)
{
    framebuffer = (volatile u8*) mbi->fb.addr;
    //...initialize other variables...
    width = SCREEN_WIDTH;
    height = SCREEN_HEIGHT;
    pitch = mbi->fb.pitch;
}

void console_putc(char ch)
{
    switch(currentState)
    {
        case NORMAL_CHARS:
            // this is where I'd put my actual putc!! IF I HAD ONE!!!
            serial_putc(ch);
            if(ch=='\e') { currentState = GOT_ESC; return; }
        case GOT_ESC:
            if(ch=='[') currentState = GOT_LBRACKET;
            else currentState = NORMAL_CHARS; 
            break;
        case GOT_LBRACKET:
            switch (ch)
            {
                case '3': { currentState = GOT_3; break; }
                case '4': { currentState = GOT_4; break; }
                case '9': { currentState = GOT_9; break; }
                case '1': { currentState = GOT_1; break; }
                default: { currentState = NORMAL_CHARS; break; }
            }
        case GOT_3:
        case GOT_4:
        case GOT_9:
            // clever fallthrough!
            switch(ch)
            {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                // and here!
                    escCharBuf = ch;
                    currentState++;
                    break;
                default: { currentState = NORMAL_CHARS; break; }
            }
        case GOT_3x:
        case GOT_4x:
        case GOT_9x:
        case GOT_1:
        case GOT_1x:
        case GOT_1xx:
            break;
    }
}

void clear_screen()
{
    u8 upper = (u8)(backgroundColor>>8);
    u8 lower = (u8)(backgroundColor<<8);
    kprintf("\nclearing the screen!\n");
    for(int i=0; i<pitch*height; i=i+2)
    {
        framebuffer[i] = lower;  
        framebuffer[i+1] = upper;
    }
}

void set_pixel(unsigned x, unsigned y, u16 color)
{
    u16* p = framebuffer + (pitch*y) + x;
    *p = color;
}