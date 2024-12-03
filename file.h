#pragma once

#define MAX_FILES 16
#define MAX_PATH 64
typedef void (*file_open_callback_t)(int fd, void* callback_data);
typedef void (*file_close_callback_t)(int errorcode, void* callback_data);
typedef void (*file_read_callback_t)(int errorcode, void* buf, unsigned numread, void* callback_data);

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  64
#define O_EXCL   128
#define O_TRUNC  512
#define O_APPEND 1024

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct File 
{
    int in_use; 
    int flags;
    char filename[MAX_PATH+1]; // null term
    unsigned offset; // position in the file
    unsigned size;
    unsigned firstCluster;
};
struct FileOpenCallbackData
{
    int fd;
    file_open_callback_t callback;
    void* callback_data;
};
struct ReadInfo
{
    int fd;
    void* buffer;
    unsigned num_requested;
    file_read_callback_t callback;
    void* callback_data;
};

void file_open(const char* filename, int flags, file_open_callback_t callback, void* callback_data);
void file_open_part_2(int errorcode, void* data, void* pfocd);
void file_close(int fd, file_close_callback_t callback, void* callback_data);

void file_read(int fd, void* buf, unsigned count, file_read_callback_t callback, void* callback_data);
void file_read_part_2(int errorcode, void* sector_data, void* callback_data);

int file_seek(int fd, int delta, int whence);
int file_tell(int fd, unsigned* offset);

