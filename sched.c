#include "utils.h"
#include "sched.h"
#include "errno.h"
#include "exec.h"
#include "memory.h"
#include "interrupt.h"

#define MAX_PROC 16
struct PCB process_table[MAX_PROC];
int current_pid = -1;
struct PageTable page_tables[MAX_PROC];
static volatile int can_schedule=0;

void sched_init(){
    //Set all state fields to VACANT
    for(int i=0; i<MAX_PROC; i++)
        process_table[i].state = VACANT;
    //Except process_table[0]...set that to STARTING
    process_table[0].state = STARTING;
}
void sched_enable()  { can_schedule=1; }

void spawn(const char* path, spawn_callback_t callback, void* callback_data)
{
    struct SpawnInfo* si = kmalloc(sizeof(struct SpawnInfo));
    if(!si)
    {
        callback(ENOMEM,-1,callback_data);
        return;
    }
    int i;
    for(i=0; i<MAX_PROC; i++)
        if(process_table[i].state == VACANT)
            break;
    if(i==MAX_PROC)
    {
        //try again later
        callback(EAGAIN,-1,callback_data);
        kfree(si);
        return;
    }
    si->pid=i;
    si->callback=callback;
    si->callback_data=callback_data;
    process_table[i].state = STARTING;

    unsigned addr = si->pid*0x400000;
    kmemset( (char*)addr, 0, 4*1024*1024 );
    exec(path, addr, spawn2, si);
}
void spawn2(int errorcode, unsigned entryPoint, void* callback_data)
{
    struct SpawnInfo* si = (struct SpawnInfo*)callback_data;
    if(errorcode)
    {
        process_table[si->pid].state = VACANT;
        si->callback( errorcode, -1, si->callback_data);
        kfree(si);
        return;
    }
    struct PCB* pcb = &process_table[si->pid];
    pcb->eax=0; pcb->ebx=0; pcb->ecx=0; pcb->edx=0;
    pcb->esi=0; pcb->edi=0; pcb->ebp=0; pcb->esp=0;
    pcb->cs = 24|3; 
    pcb->ss = 32|3;  
    pcb->esp = EXE_STACK;       
    pcb->eip = entryPoint;
    pcb->eflags = 0x202; 
    pcb->page_table = &page_tables[si->pid];

    initialize_process_page_table(&(page_tables[si->pid]), si->pid);
    process_table[si->pid].state = READY;
    si->callback( SUCCESS, si->pid, si->callback_data);
    kfree(si);
}

void initialize_process_page_table(struct PageTable* p, int pid){
    for(unsigned i=0; i < 1024; i++) 
    {
        unsigned e;

        if(i == 1)
            e = (pid << 22);
        else 
            e = (i << 22);
        e |= PAGE_MUST_BE_ONE | PAGE_WRITEABLE;

        unsigned addr = i*4*1024*1024;
        if(addr >= 4*1024*1024 && addr < 8*1024*1024)
            e |= PAGE_USER_ACCESS;
        if (addr >= (unsigned)2*1024*1024*1024)
            e |= PAGE_DEVICE_MEMORY;
        if(!(addr >= 128*1024*1024 && addr < (unsigned)2*1024*1024*1024)) 
            e |= PAGE_PRESENT;

        p->table[i] = e;
    }
}
int sched_select_process()
{
    int temp = -1;

    for(int delta=1; delta<=MAX_PROC; delta++) 
    {
        int i = (current_pid + delta) % MAX_PROC;
        if(i == 0) continue;
        if(process_table[i].state == READY || process_table[i].state == RUNNING) 
        {
            temp = i;
            break;
        }
    }
    if(temp == -1) temp++;
    return temp;
}
void sched_save_process_status(int pid, struct InterruptContext* ctx, enum ProcessState newState)
{
    if(current_pid == -1 || !current_pid)
        return;
    struct PCB* pcb = &process_table[pid];
    pcb->state = newState;
    pcb->eax = ctx->eax; pcb->ebx = ctx->ebx;
    pcb->ecx = ctx->ecx; pcb->edx = ctx->edx;
    pcb->esi = ctx->esi; pcb->edi = ctx->edi;
    pcb->ebp = ctx->ebp; pcb->esp = ctx->esp;
    pcb->cs = ctx->cs;
    pcb->ss = ctx->ss;
    pcb->eip = ctx->eip;
    pcb->eflags = ctx->eflags;
}
static int sched_first_time = 1;
void sched_restore_process_state(int pid, struct InterruptContext* ctx, enum ProcessState newState) {
    if(current_pid >= MAX_PROC)
        panic("bad PID!!");

    struct PCB* pcb = &process_table[pid];
    set_page_table(pcb->page_table);
    pcb->state = newState;
    current_pid = pid;

    if(sched_first_time) 
    {
        sched_first_time = 0;
        exec_transfer_control(SUCCESS, pcb->eip, NULL); // trace to spawn 2 and see what eip; 100,000 wrong (kernel load) 400,000 is desired
        panic("we should not get here");
    } 
    else 
    {
        ctx->eax = pcb->eax; ctx->ebx = pcb->ebx;
        ctx->ecx = pcb->ecx; ctx->edx = pcb->edx;
        ctx->esi = pcb->esi; ctx->edi = pcb->edi;
        ctx->ebp = pcb->ebp; ctx->esp = pcb->esp;
        ctx->cs = pcb->cs;
        ctx->ss = pcb->ss;
        ctx->eip = pcb->eip;
        ctx->eflags = pcb->eflags;
    }
}
void schedule(struct InterruptContext* ctx)
{
    if(!can_schedule) return;
    int new_pid = sched_select_process();
    if(new_pid == current_pid) return;
    sched_save_process_status(current_pid,ctx,READY);
    sched_restore_process_state(new_pid,ctx,RUNNING);
}