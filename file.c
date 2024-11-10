#include "file.h"
#include "errno.h"
#include "utils.h"
#include "filesys.h"


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

    if(match found)
        focd->callback(focd->fd, focd->callback_data);
    else
    {
        file_table[focd->fd].in_use = 0;
        focd->callback(ENOENT, focd->callback_data);
    }
    kfree(focd);
}

int file_open(const char* filename, int flags, file_open_callback_t callback, void* callback_data)
{
    if(kstrlen(filename) >= MAX_PATH)
    {
        callback(ENOENT, callback_data);
        return;
    }

    // get index of the File in the table
    int fd = NULL;
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
    // defer to part two. next time on shrimp OS...
    disk_read_sectors(clusterNumberToSectorNumber(2), getVbr()->sectors_per_cluster, file_open_part_2, focd);
}

void file_close(int fd, file_close_callback_t callback, void* callback_data)
{
    if(fd != valid)
        if(callback) callback(EINVAL, callback_data);

    file_table[fd].in_use = 0;
    if(callback) callback(SUCCESS, callback_data);
}

