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
        // set the firstCluster field
        file_table[focd->fd].firstCluster = (dir[find].clusterHigh<<16) | dir[find].clusterLow;
        file_table[focd->fd].size = dir[find].size;
        focd->callback(focd->fd, focd->callback_data);
    }
    else
    {
        file_table[focd->fd].in_use = 0;
        focd->callback(ENOENT, focd->callback_data);
    }
    kfree(focd);
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
    for(fd=0; fd<MAX_FILES; fd++)
        if(file_table[fd].in_use == 0)
            break;
    if(fd==MAX_FILES)
    {
        callback(EMFILE, callback_data);
        return;
    }
    file_table[fd].in_use = 1;
    file_table[fd].offset = 0;
    

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
    {
        if(callback) callback(EINVAL, callback_data);
        return;
    }
    file_table[fd].in_use = 0;
    if(callback) callback(SUCCESS, callback_data);
}

void file_read_part_2(int errorcode, void* sector_data, void* callback_data)
{

    struct ReadInfo* ri = (struct ReadInfo*) callback_data;
    if( errorcode ){
        ri->callback( errorcode, ri->buffer, 0, ri->callback_data );
    } else {
        int fd = ri->fd;
        //We assume cluster is always 4KB for this class
        unsigned offsetInBuffer = file_table[fd].offset % 4096;
        unsigned bytesLeftInBuffer = 4096-offsetInBuffer;
        unsigned bytesLeftInFile = file_table[fd].size - file_table[fd].offset;
        unsigned numToCopy = min( ri->num_requested, min( bytesLeftInBuffer,bytesLeftInFile));

        kmemcpy(
            ri->buffer,
            sector_data + offsetInBuffer,
            numToCopy
        );
        file_table[fd].offset += numToCopy;
        ri->callback(
            SUCCESS,
            ri->buffer,
            numToCopy,
            ri->callback_data
        );
    }
    kfree(ri);      //important!
    //sector_data be freed by disk_read_sectors
    return;
}


void file_read( int fd, void* buf, unsigned count, file_read_callback_t callback, void* callback_data)
{
    if(fd < 0 || fd >= MAX_FILES || file_table[fd].in_use == 0 ){
        callback(EINVAL,buf,0,callback_data);
        return;
    }
    if( count == 0 ){
        callback(SUCCESS,buf,0,callback_data);
        return;
    }

    if( file_table[fd].offset >= file_table[fd].size ){
        callback(SUCCESS,buf,0,callback_data);
        return;
    }

    struct ReadInfo* ri = kmalloc( sizeof(struct ReadInfo) );
    if(!ri){
        callback(ENOMEM, buf, 0, callback_data);
        return;
    }
    ri->fd = fd;
    ri->buffer = buf;
    ri->num_requested=count;
    ri->callback=callback;
    ri->callback_data=callback_data;

    u32* fat = getFat();
    unsigned c = file_table[fd].firstCluster;
    unsigned clustersToSkip = file_table[fd].offset / 4096;
    while(clustersToSkip > 0 ){
        c = fat[c];
        --clustersToSkip;
    }
    unsigned secnum = clusterNumberToSectorNumber(c);
    disk_read_sectors( secnum, getVbr()->sectors_per_cluster, file_read_part_2, ri );
}
int file_seek(int fd, int delta, int whence)
{

    if( fd < 0 || fd >= MAX_FILES || file_table[fd].in_use == 0 || whence < 0 || whence > 2)
        return EINVAL;

    if(whence == SEEK_SET)
    {
        if( delta < 0 )
            return EINVAL;
        file_table[fd].offset = delta;
        return SUCCESS;
    }

    if( whence == SEEK_CUR )
    {
        unsigned newOffset = file_table[fd].offset + delta;
        if( delta < 0 )
            if(newOffset >= file_table[fd].offset)
                return EINVAL;
        if( delta > 0 ){
            if( newOffset <= file_table[fd].offset )
                return EINVAL;
        }
        file_table[fd].offset = newOffset;
        return SUCCESS;
    }

    if( whence == SEEK_END ){
        unsigned newOffset = file_table[fd].size + delta;
        if( delta < 0 ){
            if( newOffset >= file_table[fd].size )
                return EINVAL;
        }
        if( delta > 0 ){
            if( newOffset <= file_table[fd].size )
                return EINVAL;
        }
        file_table[fd].offset = newOffset;
        return SUCCESS;
    }
    return EINVAL;
}
int file_tell(int fd, unsigned* offset)
{
    if( fd < 0 || fd >= MAX_FILES || file_table[fd].in_use == 0 )
        return EINVAL;
    if( !offset )
        return EINVAL;
    *offset = file_table[fd].offset;
    return SUCCESS;
}