#include "sbi.h"
#include "proc.h"
#include "syscall.h"

extern struct task_struct* current; 

void deal_syscall(struct pt_regs* regs){
    switch(regs->a7){
        case SYS_WRITE:
            if(regs->a0){
                for(int i=0; i < regs->a2; ++i){
                    char* p = (char *)regs->a1;
                    sbi_ecall(SBI_PUTCHAR, 0, p[i], 0, 0, 0, 0, 0);
                }
            }
            break;
        case SYS_GETPID:
            regs->a0 = current->pid;
            break;
        default:
            break;
    }
}