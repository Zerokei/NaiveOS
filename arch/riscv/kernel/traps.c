#include<printk.h>
#include<syscall.h>
#include<proc.h>
extern int clock_set_next_event();
extern void deal_syscall();
void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略
    switch(scause) {
        case 0x8000000000000005: // Supervisor timer interrupt
            clock_set_next_event();
            do_timer();
            break;
        case 0x0000000000000008:
            deal_syscall(regs);
            // sepc = sepc + 4
            uint64 temp;
            asm volatile ("mv t0, %[old_sepc]\n"
                  "addi t0, t0, 4\n"
                  "mv %[new_sepc], t0\n"
                : [new_sepc] "=r" (temp)
                : [old_sepc]"r" (sepc)
                : );
            regs->sepc = temp;
        default:
            break;
    }
    
}