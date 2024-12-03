#include "file.h"
#include "utils.h"
#include "disk.h"
#include "kprintf.h"
#include "memory.h"
#include "errno.h"

#define DO_LONG_NAMES 1
#define DO_CTIME 1

#pragma pack(push,1)
struct DirEntry {
    char base[8];
    char ext[3];
    u8 attributes;
    u8 reserved;
    u8 creationTimeCentiseconds;
    u16 creationTime;
    u16 creationDate;
    u16 lastAccessDate;
    u16 clusterHigh;
    u16 lastModifiedTime;
    u16 lastModifiedDate;
    u16 clusterLow;
    u32 size;
};
#pragma pack(pop)

#pragma pack(push,1)
struct LFNEntry {
    unsigned char sequenceNumber;
    char name0[10];             //5 characters
    char attributes;            //always 15
    char zero;                  //always zero
    char checksum;
    char name1[12];             //6 characters
    unsigned short alsozero;    //always zero
    char name2[4];              //2 characters
};
#pragma pack(pop)


#define MAX_PATH 64

 struct File{
     int in_use;
     int flags;
     unsigned offset;
     unsigned size;
     unsigned firstCluster;
     char filename[MAX_PATH+1];

};

#define MAX_FILES 16		//real OS's use something
                            //like 1000 or so...
struct File file_table[MAX_FILES];


struct FileOpenCallbackData{
    int fd;             //index in file_table
    file_open_callback_t callback;  //was passed into file_open
    void* callback_data;     //was passed into file_open
};

void file_open_part_2(
        int errorcode,
        void* data,
        void* pFileOpenCallbackData
){
    struct FileOpenCallbackData* d = (struct FileOpenCallbackData*) pFileOpenCallbackData;

    if( errorcode != 0 ){
        goto open_done;
    }

    //we'll set this to success if and when we find the filename
    errorcode = ENOENT;

    char* filename = file_table[d->fd].filename;
    char base[8];
    char ext[3];

    int i=0;
    int j=0;

    for(i=0;i<8;++i,++j){
        if( filename[i] == ' ' ){
            //can't possibly match
            goto open_done;
        }

        if( filename[i] == 0 || filename[i] == '.' )
            break;
        else
            base[j] = toupper(filename[i]);
    }

    //when we get here, one of these is true:
    //  filename[i] == \0
    //  filename[i] == .
    //  i == 8
    //If i is i, then we can only proceed if
    //filename[i] is either a dot or a null, indicating
    //that we're done with the base. If it's any other
    //character, the base is 9 or more characters long,
    //and we will reject the filename.

    if( i == 8 && filename[i] != '.' && filename[i] != 0 ){
        //base part is too long
        //leave foundIt as zero;
        //ignore the extension; don't bother scanning
        //disk entries
        goto open_done;
    }

    //pad base out with spaces
    while(j<8){
        base[j++] = ' ';
    }

    j=0;
    if( filename[i] == '.' ){
        i++;        //skip the dot
        for(j=0;j<3;++i,++j){
            if( filename[i] == 0 )
                break;
            else if( filename[i] == ' '){
                //reject
                goto open_done;
            }
            else
                ext[j] = toupper(filename[i]);
        }
    }

    //if we're not at the end of the filename,
    //it must have an overlong extension, so reject it
    if( filename[i] != '\0' ){
        goto open_done;
    }

    while(j<3){
        ext[j++]=' ';
    }

    struct DirEntry* de = (struct DirEntry*) data;
    while(1){
        if( !de->base[0]){
            //end of list; for our labs, we assume
            //there's always an entry with a leading zero.
            break;
        }
        else if( 0 == kmemcmp(de->base,base,8) && 0 == kmemcmp(de->ext,ext,3) ){
            errorcode = SUCCESS;
            file_table[d->fd].size = de->size;
            file_table[d->fd].firstCluster = (de->clusterHigh << 16) | de->clusterLow;
            break;
        } else {
            de++;
        }
    }

open_done:
    if( errorcode != SUCCESS ){
        //not found
        file_table[d->fd].in_use=0;
        d->callback( errorcode, d->callback_data );
    } else {
        //copy data from de to file table
        d->callback( d->fd, d->callback_data );
    }
    kfree(d);

}

void file_open(const char* filename, int flags,
               file_open_callback_t callback, void* callback_data)
{

    if( kstrlen( filename ) >= MAX_PATH ){
        callback(ENOENT, callback_data);  //No such directory entry
        return;
    }

    int fd;
    for(fd=0;fd<MAX_FILES;++fd){
        if( file_table[fd].in_use == 0 )
            break;
    }

    if( fd == MAX_FILES ){
        //EMFILE = too many open files
        callback(EMFILE, callback_data);
        return;
    }

    file_table[fd].in_use=1;
    file_table[fd].offset=0;
    file_table[fd].flags = flags;
    kstrcpy( file_table[fd].filename, filename );

    struct FileOpenCallbackData* d = (struct FileOpenCallbackData*) kmalloc(
                        sizeof(struct FileOpenCallbackData)
    );
    if(!d){
        file_table[fd].in_use=0;
        callback(ENOMEM, callback_data);
        return;
    }
    d->fd=fd;
    d->callback=callback;
    d->callback_data=callback_data;
    disk_read_sectors(
        clusterNumberToSectorNumber(2), //root directory
        sectorsPerCluster(),
        file_open_part_2,
        d);

}

void file_close(int fd, file_close_callback_t callback, void* callback_data)
{
    if( fd < 0 || fd >= MAX_FILES || file_table[fd].in_use == 0 ){
        if(callback)
            callback(EINVAL, callback_data);
        return;
    }
    file_table[fd].in_use = 0;
    if(callback)
        callback(SUCCESS, callback_data);
    return;
}


struct ReadInfo{
    int fd;
    void* buffer;
    unsigned num_requested;     //amount of data requested
    file_read_callback_t callback;
    void* callback_data;
};


void file_read_part_2(
    int errorcode,
    void* sector_data,
    void* callback_data
){

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


void file_read( int fd,         //file to read from
                void* buf,      //buffer for data
                unsigned count, //capacity of buf
                file_read_callback_t callback,
                void* callback_data //passed to callback
){

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

    unsigned c = file_table[fd].firstCluster;
    unsigned clustersToSkip = file_table[fd].offset / 4096;
    while(clustersToSkip > 0 ){
        c = fat[c];
        --clustersToSkip;
    }
    unsigned secnum = clusterNumberToSectorNumber(c);

    disk_read_sectors( secnum, sectorsPerCluster(), file_read_part_2, ri );
}




int printLfnPiece( char* c, int sz ){
    int i;
    for(i=0;i<sz;i+=2){
        if( c[i] == 0 )
            return 1;
        kprintf("%c",c[i]);
    }
    return 0;
}


void listRootCallback( int errorcode, void* sectorData, void* unused)
{

    kprintf("\nSTART\n");

    struct DirEntry* de = (struct DirEntry*) sectorData;
    while(1){
        if( de->base[0] == 0 )
            break;

        if( de->base[0] == (char)0xe5 ){
            ++de;
            continue;
        }
        if( de->attributes == 15 ){
            #if DO_LONG_NAMES
            //lfn
                struct LFNEntry* lfn = (struct LFNEntry*) de;
                int i=0;
                while( lfn[i].attributes == 15 )
                    ++i;
                int j;
                for(j=i-1;j>=0;j--){
                    int rv;
                    rv = printLfnPiece( lfn[j].name0, sizeof(lfn[j].name0) );
                    if( rv )
                        break;
                    rv = printLfnPiece( lfn[j].name1, sizeof(lfn[j].name1) );
                    if( rv )
                        break;
                    rv = printLfnPiece( lfn[j].name2, sizeof(lfn[j].name2) );
                    if( rv )
                        break;
                }
                de += i;
            #else
                de++;
                continue;   //don't try to print other metadata for LFN
            #endif
        } else {
            int i=0;
            for(i=0;i<8 && de->base[i] != ' ';++i){
                kprintf("%c",de->base[i]);
            }
            kprintf(".");
            for(i=0;i<3 && de->ext[i] != ' ';++i){
                kprintf("%c",de->ext[i]);
            }
        }


        #if DO_CTIME
            int year = 1980+(de->creationDate>>9);
            int month = (de->creationDate >> 5) & 0xf;
            int day = de->creationDate & 0x1f;
            int hour = (de->creationTime >> 11) & 0x1f;
            int minute = (de->creationTime >> 5) & 0x3f;
            int second = (de->creationTime) & 0x1f;
            int creationTimeCentiseconds = (de->creationTimeCentiseconds);
            second *= 2;
            if( creationTimeCentiseconds >= 50 && creationTimeCentiseconds <= 149 )
                second++;
            else if( creationTimeCentiseconds >= 150 ){
                second += 2;
            }
            kprintf(" %4d-%02d-%02d %02d:%02d:%02d", year,month,day,hour,minute,second);
        #endif

        kprintf("\n");

        de++;
    }

    kprintf("\nDONE\n");


}


void listRoot(){

    unsigned sn = clusterNumberToSectorNumber(2);

    //assume cluster is 4KB
    disk_read_sectors(sn,8,listRootCallback,NULL);

}

int file_seek(int fd, int delta, int whence)
{

    if( fd < 0 || fd >= MAX_FILES || file_table[fd].in_use == 0 || whence < 0 || whence > 2)
        return EINVAL;

    if( whence == SEEK_SET ){
        if( delta < 0 )
            return EINVAL;
        file_table[fd].offset = delta;
        return SUCCESS;
    }

    if( whence == SEEK_CUR ){
        unsigned newOffset = file_table[fd].offset + delta;
        if( delta < 0 ){
            if( newOffset >= file_table[fd].offset )
                return EINVAL;
        }
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


struct ReadFullyInfo{
    int fd;             //file descriptor
    char* buf;          //destination buffer
    unsigned readSoFar; //how many bytes we've read so far
    unsigned capacity;  //buffer capacity: number to read
    file_read_callback_t callback;  //callback when done reading
    void* callback_data;        //data for callback
};

void file_read_fully2(int errorcode, void* buf, unsigned numread, void* callback_data)
{
    struct ReadFullyInfo* rfi = (struct ReadFullyInfo*)callback_data;

    if( errorcode ){
        rfi->callback(errorcode, rfi->buf, rfi->readSoFar, callback_data);
        kfree(rfi);
        return;
    }
    rfi->readSoFar += numread;
    if( numread == 0 && rfi->readSoFar < rfi->capacity ){
        rfi->callback(ENODATA, rfi->buf, rfi->readSoFar, rfi->callback_data);
        kfree(rfi);
        return;
    }

    if( rfi->readSoFar == rfi->capacity ){
        rfi->callback(SUCCESS, rfi->buf, rfi->readSoFar, rfi->callback_data );
        kfree(rfi);
        return;
    }
    file_read(
        rfi->fd,
        rfi->buf+rfi->readSoFar,
        rfi->capacity - rfi->readSoFar,
        file_read_fully2,
        rfi
    );
}

void file_read_fully(int fd, void* buf, unsigned count,
                     file_read_callback_t callback,
                     void* callback_data){

    struct ReadFullyInfo* rfi = kmalloc(sizeof(struct ReadFullyInfo));
    if(!rfi){
        callback(ENOMEM, buf, 0, callback_data );
        return;
    }
    rfi->fd = fd;
    rfi->buf = (char*) buf;
    rfi->readSoFar = 0;
    rfi->capacity=count;
    rfi->callback=callback;
    rfi->callback_data=callback_data;
    file_read( fd, buf, count, file_read_fully2, rfi );
}
