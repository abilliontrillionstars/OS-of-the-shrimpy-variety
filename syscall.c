#include "syscall.h"
#include "interrupt.h"
#include "syscalldefs.h"
#include "kprintf.h"
#include "errno.h"
#include "sched.h"

void syscall_handler(struct InterruptContext* ctx){
    unsigned req = ctx->eax;
    switch(req){
        case SYSCALL_TEST:
            kprintf("Syscall test! %d %d %d\n",
                ctx->ebx, ctx->ecx, ctx->edx );
            ctx->eax = ctx->ebx + ctx->ecx + ctx->edx;
            break;
        case SYSCALL_WRITE:
        {
            int fd = (int) ctx->ebx;
            if( fd == 0 ){
                ctx->eax = ENOSYS;
                return;
            } else if( fd == 1 || fd == 2 ){
                char* buf = (char*) ctx->ecx;
                unsigned count = ctx->edx;
                for(unsigned i=0;i<count;++i){
                    kprintf("%c",buf[i]);
                }
                ctx->eax = count;
                return;
            } else {
                ctx->eax = ENOSYS;
                return;
            }
        }
        case SYSCALL_SLEEP:
        {
            unsigned howLong = ctx->ebx;
            ctx->eax = 0;   //return value from syscall
            sched_put_to_sleep_for_duration(howLong, ctx);
            schedule(ctx);
            //we are now in the new process's context
            return;
        }
        default:
            ctx->eax = ENOSYS;
    }
}
