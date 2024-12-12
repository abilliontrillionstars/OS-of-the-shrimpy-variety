#include "utils.h"
#include "interrupt.h"
#pragma once


enum WaitingFor  { NOTHING, TIME };
enum ProcessState { VACANT, READY, RUNNING, SLEEPING, STARTING };
struct PCB 
{
    enum ProcessState state;
    u32 eax,ebx,ecx,edx,esi,edi,ebp,esp;
    u32 cs;
    u32 ss;
    u32 eip;
    u32 eflags;
    struct PageTable* page_table;
    enum WaitingFor waitingFor;
    union WD {
        unsigned waitTime;      //time when process will wake up
    } waitData;
};

typedef void(*spawn_callback_t)(int errorcode, int pid, void* callback_data);
struct SpawnInfo{
    spawn_callback_t callback;
    void* callback_data;
    int pid;
};

void sched_enable();
void sched_init();

void spawn(const char* path, spawn_callback_t callback, void* callback_data);
void spawn2(int errorcode, unsigned entryPoint, void* callback_data);

void initialize_process_page_table(struct PageTable* p, int pid);
int sched_select_process();
void sched_save_process_status(int pid, struct InterruptContext* ctx, enum ProcessState newState);
void sched_restore_process_state(int pid, struct InterruptContext* ctx, enum ProcessState newState);
void schedule(struct InterruptContext* ctx);

void sched_put_to_sleep_for_duration(unsigned howLong, struct InterruptContext* ctx);
void sched_check_wakeup();


