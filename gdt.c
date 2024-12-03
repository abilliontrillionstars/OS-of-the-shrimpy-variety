#include "gdt.h"
#include "utils.h"

#pragma pack(push,1)
struct GDTEntry{
    u16 limitBits0to15;
    u16 baseBits0to15;
    u8 baseBits16to23;
    u8 flags;
    u8 flagsAndLimitBits16to19;
    u8 baseBits24to31;
};
#pragma pack(pop)


#pragma pack(push,1)
struct TaskStateSegment{
    u32 mustBeZero;
    u32 esp;
    u32 ss;     //stack segment
    u32 unused[15];
};
#pragma pack(pop)


#pragma pack(push,1)
struct LGDT{
    u16 size;
    struct GDTEntry* addr;
};
#pragma pack(pop)

//global variable
static struct GDTEntry gdt[6];


static struct TaskStateSegment tss = {
    0,
    0x10000,    //64KB mark: Stack address
    16          //offset of GDT entry for stack segment
};


#define SEGMENT_PRESENT 0x80
#define SEGMENT_RING_0 0x00
#define SEGMENT_RING_3 0x60
#define SEGMENT_IS_CODE_OR_DATA 0x10
#define SEGMENT_TYPE_DATA 2
#define SEGMENT_TYPE_CODE 10
#define SEGMENT_32_BIT 0x40
#define SEGMENT_LIMIT_32_BIT 0x80
#define SEGMENT_LIMIT_HIGH_NIBBLE 0xf

void gdt_init(){

    gdt[0].limitBits0to15 = 0;
    gdt[0].baseBits0to15 = 0;
    gdt[0].baseBits16to23 = 0;
    gdt[0].flags = 0;
    gdt[0].flagsAndLimitBits16to19 = 0;
    gdt[0].baseBits24to31 = 0;

    gdt[1].baseBits0to15=0;
    gdt[1].baseBits16to23=0;
    gdt[1].baseBits24to31=0;
    gdt[1].limitBits0to15=0xffff;
    gdt[1].flags = SEGMENT_TYPE_CODE |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_0 |
                   SEGMENT_PRESENT ;
    gdt[1].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;

    gdt[2].baseBits0to15=0;
    gdt[2].baseBits16to23=0;
    gdt[2].baseBits24to31=0;
    gdt[2].limitBits0to15=0xffff;
    gdt[2].flags = SEGMENT_TYPE_DATA |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_0 |
                   SEGMENT_PRESENT ;
    gdt[2].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;



    gdt[3].baseBits0to15=0;
    gdt[3].baseBits16to23=0;
    gdt[3].baseBits24to31=0;
    gdt[3].limitBits0to15=0xffff;
    gdt[3].flags = SEGMENT_TYPE_CODE |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_3 |
                   SEGMENT_PRESENT ;
    gdt[3].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;

    gdt[4].baseBits0to15=0;
    gdt[4].baseBits16to23=0;
    gdt[4].baseBits24to31=0;
    gdt[4].limitBits0to15=0xffff;
    gdt[4].flags = SEGMENT_TYPE_DATA |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_3 |
                   SEGMENT_PRESENT ;
    gdt[4].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;

    u32 tssAddr = (u32)(&tss);
    gdt[5].baseBits0to15=tssAddr & 0xffff;
    gdt[5].baseBits16to23=(tssAddr>>16) & 0xff;
    gdt[5].baseBits24to31=(tssAddr>>24) & 0xff;
    gdt[5].limitBits0to15=sizeof(tss)-1;
    gdt[5].flags = 0x89;
    gdt[5].flagsAndLimitBits16to19 = 0;

    struct LGDT lgdt;
    lgdt.size = sizeof(gdt);
    lgdt.addr = &gdt[0];
    void* tmp = &lgdt;
    asm volatile (
         "lgdt (%%eax)\n"       //load gdt register
          "ltr %%bx\n"          //load task register
         "mov $16,%%eax\n"      //set eax to 16: Offset to gdt[2]
         "mov %%eax,%%ds\n"     //store 16 to ds
         "mov %%eax,%%es\n"     //store 16 to es
         "mov %%eax,%%fs\n"     //store 16 to fs
         "mov %%eax,%%gs\n"     //store 16 to gs
         "mov %%eax,%%ss\n"     //store 16 to ss
         "jmpl $0x8,$reset_cs\n"    //Intel says we must do a jmp here
         "reset_cs:\n"
         "nop\n"                //no operation
         : "+a"(tmp)
         : "b"( 40 | 3 )    //TSS GDT entry or'd with ring level (3)
         : "memory"
    );

}
