#pragma once

typedef void (*file_open_callback_t)(int fd, void* callback_data);

void file_open(const char* fname, int flags,
                    file_open_callback_t callback,
                    void* callback_data);



typedef void (*file_close_callback_t)( int errorcode, void* callback_data);
void file_close(int fd, file_close_callback_t callback, void* callback_data);


typedef void (*file_read_callback_t)(
        int ecode,          //error code
        void* buf,          //buffer with data
        unsigned numread,   //num read
        void* callback_data //user data
);

void file_read( int fd,         //file to read from
                void* buf,      //buffer for data
                unsigned count, //capacity of buf
                file_read_callback_t callback,
                void* callback_data //passed to callback
);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int file_seek(int fd, int delta, int whence);

int file_tell(int fd, unsigned* offset);


void file_read_fully(int fd, void* buf, unsigned count,
                     file_read_callback_t callback,
                     void* callback_data);
