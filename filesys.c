#include "filesys.h"
#include "utils.h"
#include "errno.h"
#include "disk.h"
#include "kprintf.h"
#include "console.h"
#include "file.h"

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

int getFromRootDirByName(struct DirEntry* root, char* name) 
{
    //kprintf("\nfile to search for: '%s'\n", name);
    // format the target's filename to the rootdir 8.3 (maybe)
    for(int i=0; name[i]; i++)
        name[i] = toUpper(name[i]);

    int dot_i = kstrstr_index(name, ".");
    if(dot_i == -1)
        // tripwire hook! filename didn't have a dot in it, so it can't be valid.
        return -1;
    if(dot_i+4 < kstrlen(name))
        // tripwire hook! extension is longer than 3 characters.
        return -1;
    if(kstrstr_index(name, " ") != -1)
        // spaces aren't allowed!
        return -1;

    int index;
    for(index=0; (root+index)->base[0] || (root+index)->attributes == 15; index++)
    {
        struct DirEntry* entry = root+index;
        //concat the filename
        char buffer[13];
        int i=0;
        for(int j=0; j<8; j++)
            if(entry->base[j]==' ')
                break;
            else
            {
                buffer[i] = entry->base[j];
                i++;
            }
        buffer[i] = '.';
        i++;
        for(int j=0; j<3; j++)
            if(entry->ext[j]==' ')
                break;
            else
            {
                buffer[i] = entry->ext[j];
                i++;
            }
        buffer[i] = '\0';

        //kprintf("\nchecking %s against file %s...", name, buffer);
        if(kstrequals(buffer, name))
            return index;
    }
    return -1;
}

unsigned clusterNumberToSectorNumber(unsigned clnum) 
{
    if(!vbr.checksum) // no vbr?
        return (unsigned) kprintf("VBR not yet initialized, doesn't exist, or has otherwise invalid checksum. returning...\n"); // code golf!

    //kprintf("sectors per cluster is %d.\n", vbr.sectors_per_cluster);
    return clnum/vbr.sectors_per_cluster;
}

struct VBR* getVbr()  { return &vbr; }

