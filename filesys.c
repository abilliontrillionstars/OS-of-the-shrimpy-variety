#include "filesys.h"
#include "utils.h"
#include "errno.h"
#include "disk.h"
#include "kprintf.h"

struct GUID efiBootGUID = {0xc12a7328, 0xf81f, 0x11d2, 0xba4b, {0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b}};
struct GUID windowsDataGUID = {0xebd0a0a2, 0xb9e5, 0x4433, 0x87c0, {0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7}};
struct GUID linuxGUID = {0x0fc63daf, 0x8483, 0x4772, 0x8e79, {0x3d, 0x69,0xd8, 0x47, 0x7d, 0xe4}};
struct VBR vbr;

static void read_vbr_callback(int errorcode, void* sectorData, void* kmain_callback)
{
    if(errorcode != SUCCESS)
    {
        kprintf("Cannot read VBR: %d\n",errorcode);
        panic("Cannot continue");
        return;
    }
    kmemcpy(&vbr,sectorData,sizeof(vbr));
    disk_metadata_callback_t f = (disk_metadata_callback_t) kmain_callback;
    f();
}
static void read_partition_table_callback(int errorcode, void* sectorData, void* kmain_callback)
{
    if(errorcode != SUCCESS)
    {
        kprintf("Cannot read partition table: %d\n",errorcode);
        panic("Cannot continue");
    }
    struct GPTEntry* entry = (struct GPTEntry*) sectorData;
    disk_read_sectors(entry->firstSector, 1, read_vbr_callback, kmain_callback);
}

void disk_read_metadata(disk_metadata_callback_t kmain_callback)
{
    disk_read_sectors(2, 1, read_partition_table_callback, kmain_callback);
}


unsigned clusterNumberToSectorNumber(unsigned clnum) 
{
    // vbr.sectorsPerCluster
    if(!vbr.checksum) // no vbr?
        return (unsigned) kprintf("VBR not yet initialized, doesn't exist, or has otherwise invalid checksum. returning...\n"); // code golf!
    
    //kprintf("sectors per cluster is %d.\n", vbr.sectors_per_cluster);
    return clnum/vbr.sectors_per_cluster;
}

