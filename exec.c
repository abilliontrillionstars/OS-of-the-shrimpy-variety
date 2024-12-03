#include "exec.h"
#include "file.h"
#include "errno.h"
#include "utils.h"
#include "kprintf.h"
#include "memory.h"
#include "utils.h"

#pragma pack(push,1)
struct DOSHeader{
    char magic[2];          //'MZ'
    u16 lastsize;
    u16 blockCount;
    u16 numRelocations;
    u16 headerSize;
    u16 minimumAlloc;
    u16 maximumAlloc;
    u16 stackSegment;
    u16 stackPointer;
    u16 checksum;
    u16 entryPoint;         //for DOS stub
    u16 codeSegment;
    u16 relocation;
    u16 numOverlays;
    char reserved[8];
    u16 oemID;
    u16 oemInfo;
    char reserved2[20];
    u32 peOffset;           //we need this
};
#pragma pack(pop)


#pragma pack(push,1)
struct COFFHeader{
     u16 machineType; //0x14c=i386, 0x8664=x64,
                      //0x1c0=arm32, 0xaa64=arm64
     u16 numSections;
     u32 time;
     u32 symbolTableStart;
     u32 numSymbols;
     u16 optionalHeaderSize;
     u16 flags;               //bit 2=exe, bit 14=dll
};
#pragma pack(pop)


#pragma pack(push,1)
struct PEDirectory{
    u32 offset;
    u32 size;
};
#pragma pack(pop)

#pragma pack(push,1)
struct OptionalHeader{
    u16 magic;             //0x010b=32 bit, 0x020b=64 bit
    u16 linkerVersion;
    u32 codeSize;
    u32 dataSize;
    u32 bssSize;
    u32 entryPoint;        //relative to imageBase
    u32 codeBase;
    u32 dataBase;
    u32 imageBase;         //where exe should be loaded
    u32 sectionAlign;
    u32 fileAlign;
    u32 osVersion;
    u32 imageVersion;
    u32 subsystemVersion;
    u32 winVersion;
    u32 imageSize;
    u32 headerSize;
    u32 checksum;
    u16 subsystem;          //0=unknown, 1=native,
                            //2=gui, 3=console, 7=posix
    u16 dllCharacteristics;
    u32 stackReserve;
    u32 stackCommit;
    u32 heapReserve;
    u32 heapCommit;
    u32 loadFlags;
    u32 numDirectories;     //always 16
    struct PEDirectory directories[16];
};
#pragma pack(pop)


#pragma pack(push,1)
struct PEHeader{
    char magic[4];
    struct COFFHeader coffHeader;
    struct OptionalHeader optionalHeader;
};
#pragma pack(pop)


#pragma pack(push,1)
struct SectionHeader{
    char name[8];
    u32 sizeInRAM;     //size in RAM
    u32 address;       //offset of section from imageBase in RAM
    u32 sizeOnDisk;    //size of data on disk
    u32 dataPointer;   //offset from start of file to first page
    u32 relocationPointer; //file pointer for relocation entries
    u32 lineNumberPointer;
    u16 numRelocations;
    u16 numLineNumbers;
    u32 characteristics;  //flag bits (next slide)
};
#pragma pack(pop)



#define SECTION_CODE (1<<5)
#define SECTION_DATA (1<<6)
#define SECTION_UNINITIALIZED_DATA (1<<7)
#define SECTION_READABLE (1<<30)
#define SECTION_WRITABLE (1<<31)

#define MAX_SECTIONS 32

struct ExecInfo{
    int fd;
    unsigned loadAddress;
    exec_callback_t callback;
    void* callback_data;
    struct SectionHeader sectionHeaders[MAX_SECTIONS];
    int numSectionsLoaded;
    struct DOSHeader dosHeader;
    struct PEHeader peHeader;
};

#define EXE_STACK 0x800000


void exec_transfer_control(int errorcode, unsigned entryPoint, void* callback_data)
{
    if( errorcode ){
        kprintf("exec failed: %d\n",errorcode);
        return;
    }
    u32 c = EXE_STACK;
    u32 b = entryPoint;
    asm volatile(
        "mov $(32|3),%%eax\n"   //ring 3 GDT data segment
        "mov %%ax,%%ds\n"       //ring 3 GDT data segment
        "mov %%ax,%%es\n"       //ring 3 GDT data segment
        "mov %%ax,%%fs\n"       //ring 3 GDT data segment
        "mov %%ax,%%gs\n"       //ring 3 GDT data segment
        "pushl $(32|3)\n"       //ss, ring 3 GDT data segment
        "push %%ecx\n"          //esp
        "push $0x202\n"         //eflags
        "pushl $(24|3)\n"       //cs, ring 3 GDT code segment
        "pushl %%ebx\n"         //eip
        "xor %%eax,%%eax\n"     //clear register
        "xor %%ebx,%%ebx\n"     //clear register
        "xor %%ecx,%%ecx\n"     //clear register
        "xor %%edx,%%edx\n"     //clear register
        "xor %%esi,%%esi\n"     //clear register
        "xor %%edi,%%edi\n"     //clear register
        "xor %%ebp,%%ebp\n"     //clear register
        "iret\n"
        : "+c"(c), "+b"(b)
    );
}

void doneLoading(struct ExecInfo* ei)
{

    u32 entryPoint = ei->peHeader.optionalHeader.imageBase +
                     ei->peHeader.optionalHeader.entryPoint;
    exec_callback_t callback = ei->callback;
    void* callback_data = ei->callback_data;
    file_close(ei->fd, NULL, NULL);
    kfree(ei);
    callback(SUCCESS, entryPoint, callback_data );
    return;
}

void exec6( int errorcode, void* buf, unsigned nr, void* callback_data);


void loadNextSection(struct ExecInfo* ei){

    //shorten things up
    int i = ei->numSectionsLoaded;

    if( i == ei->peHeader.coffHeader.numSections ){
        doneLoading(ei);
        return;
    }

    struct SectionHeader* s = &(ei->sectionHeaders[i]);
    unsigned flags = s->characteristics;
    if( flags & (SECTION_CODE | SECTION_DATA |
                 SECTION_UNINITIALIZED_DATA) ){
        char* p = (char*) (ei->loadAddress);
        p += s->address;
        int numToLoad = min(s->sizeInRAM,s->sizeOnDisk);
        for(unsigned i=numToLoad;i<s->sizeInRAM;i++){
            p[i] = 0;
        }
        int rv = file_seek( ei->fd, s->dataPointer, SEEK_SET ) ;
        if(rv){
            ei->callback(rv, 0, ei->callback_data);
            file_close(ei->fd,NULL,NULL);
            kfree(ei);
            return;
        }
        file_read_fully( ei->fd, p, numToLoad, exec6, ei );

    } else {
        ei->numSectionsLoaded += 1;
        loadNextSection(ei);
    }
}


void exec6( int errorcode, void* buf, unsigned nr, void* callback_data)
{
    struct ExecInfo* ei = (struct ExecInfo*) callback_data;

    if( errorcode ){
        ei->callback(errorcode, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }

    ei->numSectionsLoaded++;
    loadNextSection(ei);
    return;
}

void exec5( int errorcode, void* buf, unsigned numread, void* callback_data)
{
    struct ExecInfo* ei = (struct ExecInfo*)callback_data;
    if(errorcode){
        ei->callback(errorcode, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }
    ei->numSectionsLoaded = 0;
    loadNextSection(ei);
}

#define EXE_BASE 0x400000

void exec4( int errorcode, void* buf, unsigned numread, void* callback_data)
{
    struct ExecInfo* ei = (struct ExecInfo*)callback_data;
    if(errorcode){
        ei->callback(errorcode, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL );
        kfree(ei);
        return;
    }
    if( ei->peHeader.coffHeader.machineType != 0x14c ||
        ei->peHeader.optionalHeader.magic != 0x010b ){
        ei->callback(ENOEXEC, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }
    if( ei->peHeader.optionalHeader.imageBase != EXE_BASE ){
        ei->callback(EFAULT, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }
    if( ei->peHeader.coffHeader.numSections > MAX_SECTIONS ){
        ei->callback(ENOEXEC, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }
    file_read_fully(ei->fd, ei->sectionHeaders,
                        ei->peHeader.coffHeader.numSections * sizeof(struct SectionHeader),
                        exec5, ei );
}

void exec3( int errorcode, void* buf, unsigned numread, void* callback_data)
{
    struct ExecInfo* ei = (struct ExecInfo*)callback_data;
    if(errorcode){
        ei->callback(errorcode, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL);
        kfree(ei);
        return;
    }
    if( 0 != kmemcmp(ei->dosHeader.magic,"MZ",2) ){
        ei->callback(ENOEXEC, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL );
        kfree(ei);
        return;
    }
    int rv = file_seek(ei->fd, ei->dosHeader.peOffset, SEEK_SET);
    if(rv<0){
        ei->callback(rv, 0, ei->callback_data);
        file_close(ei->fd, NULL, NULL );
        kfree(ei);
        return;
    }
    file_read_fully( ei->fd, &(ei->peHeader),
                     sizeof(ei->peHeader),
                     exec4, ei );
}

void exec2( int fd, void* callback_data)
{
    struct ExecInfo* ei = (struct ExecInfo*)callback_data;
    if(fd < 0){
        ei->callback(fd, 0, callback_data );
        kfree(ei);
        return;
    }
    ei->fd=fd;
    file_read_fully( fd, &(ei->dosHeader),
                     sizeof(ei->dosHeader),
                     exec3, ei );
}

void exec(  const char* fname,
            unsigned loadAddress,
            exec_callback_t callback,
            void* callback_data ){

    struct ExecInfo* ei = kmalloc(sizeof(struct ExecInfo));
    if(!ei){
        callback(ENOMEM, 0, callback_data);
        return;
    }
    ei->callback=callback;
    ei->loadAddress=loadAddress;
    ei->callback_data=callback_data;
    file_open(fname, 0, exec2, ei );    //0=flags
}
