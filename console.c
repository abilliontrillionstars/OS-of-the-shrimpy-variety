#include "utils.h"
#include "console.h"
#include "serial.h"
#include "etec3701_10x20.h"

#define DO_SCROLL 1
#define DO_COLOR 1

static volatile u8* framebuffer;
static unsigned pitch;
static unsigned width,height;
static u16 foregroundColor=0b1010110101010101;
static u16 backgroundColor=0b0000000000010101;
static int row=0;
static int col=0;
static void draw_character( unsigned char ch, int x, int y);
static void clear_screen();
static int numRows;

void console_init(struct MultibootInfo* info){
    framebuffer = (volatile u8*) (info->fb.addr);
    pitch = info->fb.pitch;
    width = info->fb.width;
    height = info->fb.height;
    numRows = height/CHAR_HEIGHT;
    clear_screen();
}

static void clear_screen(){
    volatile u8* rowstart = framebuffer;
    for(int y=0;y<height;++y){
        volatile u16* p = (volatile u16*)rowstart;
        for(int x=0;x<width;++x,++p){
            *p=backgroundColor;
        }
        rowstart += pitch;
    }
}

void set_pixel(int x, int y, u16 color){
    volatile u16* p = (volatile u16*)(framebuffer + (y*pitch + x*2));
    *p = color;
}

void console_invert_pixel(unsigned x, unsigned y){
    volatile u16* p = (volatile u16*)(framebuffer + (y*pitch + x*2));
    *p ^= 0xffff;
}

void draw_character( unsigned char ch, int x, int y){
    int r,c;
    for(r=0;r<CHAR_HEIGHT;++r){
        for(c=0;c<CHAR_WIDTH;++c){
            if( font_data[ch][r] & ( (1<<(CHAR_WIDTH-1))>>c) )
                set_pixel( x+c, y+r, foregroundColor );
            else
                set_pixel( x+c, y+r, backgroundColor );
        }
    }
}

#define RGB(r,g,b) ( (r<<11) | (g<<5) | (b) )

static u16 color_low[] = {
    RGB(0,0,0),     //black
    RGB(21,0,0),    //dark red
    RGB(0,42,0),    //dark green
    RGB(21,42,0),   //olive
    RGB(0,0,21),    //royal blue
    RGB(21,0,21),   //purple
    RGB(0,42,21),   //cyan
    RGB(21,42,21)   //grey
};

static u16 color_high[] = {
    RGB(10,20,10),  //dark grey
    RGB(31,20,10),  //pink
    RGB(10,63,10),  //lime
    RGB(31,63,10),  //yellow
    RGB(10,20,31),  //light blue
    RGB(31,20,31),  //magenta
    RGB(10,63,31),  //light cyan
    RGB(31,63,31)   //white
};

void console_putc(char c){

    //-1=ordinary
    //0=have seen \e
    //1,2,3, etc. = have seen this many characters
    //color sequence:
    // \e[xxm  or \e[xxxm
    static int escapeMode=-1;
    static char escapeChars[4];

    serial_putc(c);
    if( escapeMode != -1 ){
        //0=expecting [
        //1=expecting first digit
        //2=expecting second digit
        //3=expecting third digit
        //4=expecting m
        //5=error

        switch(escapeMode){
            case 0:
                if( c != '[' ){
                    escapeMode=-1;
                    return;
                }
                escapeMode++;
                return;
            case 1:
                escapeChars[0] = c;
                escapeMode++;
                return;
            case 2:
                escapeChars[1] = c;
                escapeMode++;
                return;
            case 3:
                if( c == 'm' )
                    break;
                else{
                    escapeChars[2] = c;
                    escapeMode++;
                    return;
                }
            default:
                break;
        }

        int i;
        if( escapeMode == 3 )
            i = (escapeChars[0]-'0')*10 + (escapeChars[1]-'0');
        else
            i = (escapeChars[0]-'0')*100 + (escapeChars[1]-'0')*10 + (escapeChars[2]-'0');

        if( i >= 30 && i <= 37 )
            foregroundColor = color_low[i-30];
        else if( i >= 40 && i <= 47 )
            backgroundColor = color_low[i-40];
        else if( i >= 90 && i <= 97 )
            foregroundColor = color_high[i-90];
        else if( i >= 100 && i <= 107 )
            backgroundColor = color_high[i-100];

        escapeMode=-1;
        return;
    }

    switch(c){

        #if DO_COLOR
        case '\e':
            escapeMode=0;
            escapeChars[0] = escapeChars[1] = escapeChars[2] = escapeChars[3] = '\0';
            return;
        #endif
        case '\n':
            row++;
            col=0;
            break;
        case '\r':
            col=0;
            break;
        case '\t':
            {
                int tmp = col % 8;
                int extra = 8-tmp;
                for(int i=0;i<extra;++i)
                    console_putc(' ');
                break;
            }
        case '\f':
            row=0;
            col=0;
            clear_screen();
            break;
        case 0x7f:
            if( row==0 && col==0 ){
                //nothing to do
            } else {
                col--;
                if(col<0){
                    row--;
                    col=79;
                }
                draw_character(' ',col*CHAR_WIDTH, row*CHAR_HEIGHT);
            }
            break;
        default:
            draw_character(c, col*CHAR_WIDTH, row*CHAR_HEIGHT);
            col++;
    }

    if( col == 80 ){
        col=0;
        row++;
    }

    #if DO_SCROLL
        if( row >= numRows ){
            kmemcpy(
                (void*)framebuffer,
                (void*)(framebuffer+pitch*CHAR_HEIGHT),
                (numRows-1)*pitch*CHAR_HEIGHT);
            volatile u16* p = (volatile u16*)(framebuffer+(numRows-1)*CHAR_HEIGHT*pitch);
            int y = (numRows-1)*CHAR_HEIGHT;
            for( ; y<height;++y){
                for(int x=0;x<width;++x){
                    p[x] = backgroundColor;
                }
                p = (volatile u16*)((u8*)p + pitch);
            }
            row=numRows-1;
            col=0;
        }
    #endif

}
