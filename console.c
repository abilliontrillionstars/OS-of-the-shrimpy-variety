#include "utils.h"
#include "console.h"
#include "serial.h"
#include "kprintf.h"

#include "font-default.h"
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

static unsigned cursorRow;
static unsigned cursorColumn;

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

    cursorRow = 0;
    cursorColumn = 0;
    clear_screen();
}

void console_putc(char ch)
{
    switch(currentState)
    {
        case NORMAL_CHARS:
            serial_putc(ch);
            switch(ch)
            {
                case '\t': 
                    int toTab = 8-(cursorColumn%8);
                    for(int i=0; i<toTab; i++)
                        kprintf("%c", ' ');
                    break;
                case '\e': 
                    break; //nothing for now
                case '\f':
                    cursorColumn = 0;
                    cursorRow = 0;
                    clear_screen();
                    break;
                case '\n':
                    int toLine = 80-cursorColumn;
                    for(int i=0; i<toLine; i++)
                        kprintf("%c", ' ');

                    break;    
                case '\r':
                    cursorColumn = 0;
                    break;
                case '\x7f':
                    if(cursorColumn) cursorColumn--;
                    else if(cursorRow)
                    {
                        cursorRow--;
                        cursorColumn = 79;
                    }
                    draw_character(' ', cursorColumn*CHAR_WIDTH, cursorRow*CHAR_HEIGHT);
                    break;
                default:
                    draw_character(ch, cursorColumn*CHAR_WIDTH, cursorRow*CHAR_HEIGHT);
                    cursorColumn++; 
                    if(cursorColumn == 80) 
                    {
                        cursorRow++; 
                        if(cursorRow == 30) 
                        {
                            scroll(); 
                            cursorRow--;
                        }
                        cursorColumn = 0;
                    }
                    break;
            }
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
    u8 lower = (u8)((backgroundColor<<8)>>8);
    for(int i=0; i<pitch*height; i=i+2)
    {
        framebuffer[i] = lower;  
        framebuffer[i+1] = upper;
    }
}
void set_pixel(unsigned x, unsigned y, u16 color)
{
    u8 upper = (u8)(color>>8);
    u8 lower = (u8)((color<<8)>>8);

    volatile u8* p = framebuffer+(pitch*y)+(x*2);
    *p = lower;
    *(p+1) = upper;
}
void console_invert_pixel(unsigned x, unsigned y)
{
    volatile u16* p = (volatile u16*)(framebuffer + (y*pitch + x*2));
    *p ^= 0xffff;
}

void draw_character(unsigned char ch, int x, int y)
{
    int r,c;
    unsigned idx = (unsigned)ch; //can't use char as array index
    for(r=0; r<CHAR_HEIGHT; ++r)
        for(c=0; c<CHAR_WIDTH; ++c)
            if((font_data[idx][r]>>c)&1)
                //for some reason they drew backwards? whatever
                set_pixel(x-c +CHAR_WIDTH-1, y+r, foregroundColor);
            else
                set_pixel(x-c +CHAR_WIDTH-1, y+r, backgroundColor);        
}

void scroll()
{
    kmemcpy((void*)framebuffer, (void*)framebuffer+(pitch*CHAR_HEIGHT), CHAR_HEIGHT*pitch*30);
    //kprintf("\rcolor is: %d\n", backgroundColor);
    for(int i=0; i<80; i++)
        draw_character(' ',i*CHAR_WIDTH , 30*CHAR_HEIGHT);
}

