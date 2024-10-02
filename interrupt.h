#pragma once

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