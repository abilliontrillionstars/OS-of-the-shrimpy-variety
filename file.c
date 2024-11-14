#include "file.h"
#include "errno.h"
#include "utils.h"
#include "filesys.h"
#include "kprintf.h"
#include "disk.h"
#include "memory.h"

struct File file_table[MAX_FILES];

void file_open_part_2(int errorcode, void* data, void* pfocd)
{
    struct FileOpenCallbackData* focd = (struct FileOpenCallbackData*) pfocd;
    if(errorcode != SUCCESS)
    {
        file_table[focd->fd].in_use = 0;
        focd->callback(errorcode, focd->callback_data);
        kfree(focd);
        return;
    }
      
    //find the file in the root directory
    struct DirEntry* dir = (struct DirEntry*)data;
    int find = getFromRootDirByName(dir, file_table[focd->fd].filename);



    if(find != -1) 
    {
        focd->callback(focd->fd, focd->callback_data);
        file_table[focd->fd].in_use = 1;
    }
    else
    {
        file_table[focd->fd].in_use = 0;
        focd->callback(ENOENT, focd->callback_data);
    }
    int waugh = file_table[focd->fd].in_use;
    kfree(focd);
    kprintf("wuagh: %d", waugh);
}

void file_open(const char* filename, int flags, file_open_callback_t callback, void* callback_data)
{
    if(kstrlen(filename) >= MAX_PATH)
    {
        callback(ENOENT, callback_data);
        return;
    }

    // get index of the File in the table. that is, find an empty slot in the table to use
    int fd; 
    for(fd=0; fd<=MAX_FILES; fd++)
        if(file_table[fd].in_use == 0)
            break;
    if(fd==MAX_FILES)
    {
        callback(EMFILE, callback_data);
        return;
    }
    file_table[fd].in_use = 1;

    //we've validated filename won't overflow file_table.filename
    kstrcpy(file_table[fd].filename, filename);

    struct FileOpenCallbackData* focd = (struct FileOpenCallbackData*) kmalloc(sizeof(struct FileOpenCallbackData));
    if(!focd)
    {
        file_table[fd].in_use = 0;
        callback(ENOMEM, callback_data);
        return;
    }

    focd->fd = fd;
    focd->callback = callback;
    focd->callback_data = callback_data;
    struct VBR* vbr = getVbr();
    // defer to part two. next time on shrimp OS...
    disk_read_sectors(vbr->first_sector + vbr->reserved_sectors + (vbr->num_fats * vbr->sectors_per_fat), vbr->sectors_per_cluster, file_open_part_2, focd);
}

void file_close(int fd, file_close_callback_t callback, void* callback_data)
{
    if(fd<0 || fd>=MAX_FILES || !(file_table[fd].in_use))
        if(callback) callback(EINVAL, callback_data);

    file_table[fd].in_use = 0;
    if(callback) callback(SUCCESS, callback_data);
}

