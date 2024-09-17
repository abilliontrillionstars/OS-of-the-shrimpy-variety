#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

void outb(u16 port, u8 value);
u8 inb(u16 port);

void kmemcpy(const void* start, void* dest, unsigned length);

#pragma pack(push,1)

struct MB_Meminfo
{
    u32 size;
    u32 addr;
    u32 padding;
    u32 length;
    u32 padding2;
    u32 type;
    u32 attributes;    
};
struct MB_MemoryMapping 
{
    u32 length;
    struct MB_Meminfo* addr;
};
struct MB_MemorySizes
{
    u32 lower;
    u32 upper;
};
struct MB_Drives
{
    u32 length;
    u32 addr;
};
struct MB_Config { void* ptr; };
struct MB_Loader { char* name; };
struct MB_AdvancedPowerManagement { void* ptr; };
struct MB_VideoBiosExtensions
{
    u32 control;
    u32 mode_info;
    u32 mode;
    u32 segment;
    u32 offset;
    u32 length;
};
struct MB_FrameBuffer
{
    u32 addr;
    u32 addr64;
    u32 pitch;
    u32 width;
    u32 height;
    u8 bpp;
    u8 type;
    u8 redPos;
    u8 redMask;
    u8 greenPos;
    u8 greenMask;
    u8 bluePos;
    u8 blueMask;
};

struct MultibootInfo 
{
    unsigned flags;
    struct MB_MemoryMapping map;
    struct MB_Drives drives;
    struct MB_Config config;
    struct MB_Loader loader;
    struct MB_AdvancedPowerManagement apm;
    struct MB_VideoBiosExtensions vbe;
    struct MB_FrameBuffer fb;
};

#pragma pack(pop)
