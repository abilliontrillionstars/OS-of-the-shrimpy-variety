#pragma once

#define MAX_FILES 16
#define MAX_PATH 64
typedef void (*file_open_callback_t)(int fd, void* callback_data);
typedef void (*file_close_callback_t)(int errorcode, void* callback_data);

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  64
#define O_EXCL   128
#define O_TRUNC  512
#define O_APPEND 1024

struct File 
{
    int in_use;
    int flags;
    char filename[MAX_PATH+1]; // null term

};
struct File file_table[MAX_FILES];

struct FileOpenCallbackData
{
    int fd;
    file_open_callback_t callback;
    void* callback_data;
};

int file_open(const char* filename, int flags, file_open_callback_t callback, void* callback_data);
void file_open_part_2(int errorcode, void* data, void* pfocd);
void file_close(int fd, file_close_callback_t callback, void* callback_data);

