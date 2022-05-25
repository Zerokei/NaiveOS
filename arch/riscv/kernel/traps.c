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
                  "csrw sepc, t0\n"
                : : [old_sepc]"r" (sepc)
                : );
            // regs->sepc = temp;
            break;
        case 0x000000000000000C: // Instruction Page Fault        
        case 0x000000000000000D: // Load Page Fault
        case 0x000000000000000F: // Store/AMO Page Fault
            do_page_fault(scause, regs);
            break;
        default:
            break;
    }
    
}

extern char uapp_start[];
extern char uapp_end[];
extern struct task_struct* current;
void do_page_fault(unsigned long scause, struct pt_regs *regs) {
    /*

    */
    // 1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
    uint64 bad_addr;
    asm volatile (
        "csrr %[bad_addr], stval\n"
    : [bad_addr] "=r" (bad_addr)
    : : );
    // 2. 通过 find_vm() 找到对应的 vm_area_struct
    // printk("mm=%lx\n", (current->mm)->mmap);
    struct vm_area_struct *p = find_vma(current->mm, bad_addr);
    /*3.
        通过 scause 获得当前的 Page Fault 类型 
        通过 vm_area_struct 的 vm_flags 对当前的 Page Fault 类型进行检查
        - Instruction Page Fault      -> VM_EXEC
        - Load Page Fault             -> VM_READ
        - Store Page Fault            -> VM_WRITE
    */

    printk("[S] PAGE_FAULT: scause: %ld, sepc: 0x%016lx, badaddr: 0x%016lx\n", scause, regs->sepc, bad_addr);

    switch(scause) {
    case 0x000000000000000C: // Instruction Page Fault
        if(!(p->vm_flags & VM_EXEC)){
            printk("The Page is not executable!");
        }    
        break;
    case 0x000000000000000D: // Load Page Fault
        if(!(p->vm_flags & VM_READ)){
            printk("The Page is not readable!");
        }
        break;
    case 0x000000000000000F: // Store/AMO Page Fault
        if(!(p->vm_flags & VM_WRITE)){
            printk("The Page is not writable!");
        }
        break;
    }
    // 4. 最后调用 create_mapping 对页表进行映射
    if(bad_addr <= uapp_end-PA2VA_OFFSET){
        create_mapping(current->pgd, USER_START, uapp_start-PA2VA_OFFSET, uapp_end-uapp_start, EW|EU|EX|ER|EV); // 映射uapp代码段
    }
    else if(bad_addr >= USER_END-PGSIZE){
        uint64 stack_page;
        // printk("current=%lx %lx %lx\n", current, &(current->thread_info), (current->thread_info).user_sp);
        if((current->thread_info).user_sp){//已经申请过页表了
            stack_page = (current->thread_info).user_sp;
        }
        else stack_page = kalloc(); // 申请新页
        create_mapping(current->pgd, USER_END-PGSIZE, stack_page-PA2VA_OFFSET, PGSIZE, EU|EW|ER|EV); // 映射用户栈
    }
    else {
    }
}