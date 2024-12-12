#pragma once

#define EXE_STACK 0x800000

#include "utils.h"

typedef void (*exec_callback_t)(int errorcode, unsigned entryPoint, void* callback_data);
void exec( const char* fname, unsigned loadAddress, exec_callback_t callback, void* callback_data );
void exec_transfer_control(int errorcode,
                           u32 entryPoint,
                           void* callback_data
);
