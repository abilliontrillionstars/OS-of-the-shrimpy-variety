#pragma once

#include "utils.h"

#define MAX_DISK_SIZE_MB 128
extern u32 fat[ MAX_DISK_SIZE_MB*1024*1024/4096 ];


void disk_init();
typedef void (*disk_callback_t)(int, void*, void*);
typedef void (*disk_metadata_callback_t)(void);

void disk_read_sectors(
    unsigned firstSector,
    unsigned numSectors,
    disk_callback_t callback,
    void* callback_data);

void disk_read_metadata( disk_metadata_callback_t kmain_callback );

unsigned clusterNumberToSectorNumber( unsigned clnum );
unsigned sectorsPerCluster();
