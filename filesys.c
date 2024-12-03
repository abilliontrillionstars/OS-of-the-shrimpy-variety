#include "filesys.h"
#include "utils.h"
#include "errno.h"
#include "disk.h"
#include "kprintf.h"
#include "console.h"
#include "file.h"

#define MAX_DISK_SIZE_MB 128
u32 fat[MAX_DISK_SIZE_MB*1024*1024 / 4096];

struct VBR vbr;

static void read_fat(disk_metadata_callback_t f);

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
    read_fat(f);
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

static int fatSectorsRemaining;
static disk_metadata_callback_t kmain_callback;
void read_fat_callback(int errorcode, void* data, void* p)
{
    kprintf("let's see...\n");

    if(errorcode != SUCCESS)  { panic("Cannot read FAT"); }
    u32 i = (u32) p;
    //each sector has 128 FAT entries in it
    //and is 512 bytes in size
    kmemcpy( fat + 128*i, data, 512 );
    --fatSectorsRemaining;
    if(!fatSectorsRemaining)
        kmain_callback();
}
static void read_fat(disk_metadata_callback_t f)
{
    kprintf("reading fat...\n");

    kmain_callback=f;
    fatSectorsRemaining = vbr.sectors_per_fat;
    unsigned fatStart = vbr.first_sector + vbr.reserved_sectors;

    //this fires off a bunch of overlapped reads
    for(u32 i=0;i<vbr.sectors_per_fat;++i)
        disk_read_sectors(fatStart+i, 1, read_fat_callback, (void*)i);
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
    if(clnum<2 || clnum >0xffffff)
        panic("Bad cluster number!");

    unsigned dataArea = vbr.first_sector + vbr.reserved_sectors + vbr.num_fats*vbr.sectors_per_fat;
    return dataArea + (clnum-2) * vbr.sectors_per_cluster;
}
struct VBR* getVbr()  { return &vbr; }
u32* getFat()  { return (u32*)&fat; }
