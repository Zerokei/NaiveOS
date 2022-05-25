//arch/riscv/kernel/proc.c
#include<proc.h>
#include<syscall.h>

extern void __dummy();
extern void ret_from_fork(struct pt_regs *trapframe);
extern void __switch_to(struct task_struct* prev, struct task_struct* next);

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

extern char uapp_start[];
extern char uapp_end[];
extern unsigned long swapper_pg_dir[]; // S态页表

int id; // 现在进程的编号情况

void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 taks[0] 指向 idle
    idle = (struct task_struct *)kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    idle->pgd = swapper_pg_dir;
    current = idle;
    // printk("%lx, %lx, %lx\n", idle, &(idle->thread_info), &(idle->thread));
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`, 
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， `sp` 设置为 该线程申请的物理页的高地址

    int i;
    id = 1;
    for(i = 1; i < 2/*NR_TASKS*/; ++i){
        task[i] = (struct task_struct *)kalloc();
        task[i]->pid = i;
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        
        (task[i]->thread).ra = (uint64)__dummy;
        (task[i]->thread).sp = (uint64)task[i] + PGSIZE - 8; //设置sp

        task[i]->pgd = (pagetable_t) kalloc();

        int j;
        for(j = 0; j < 512; ++j)
            task[i]->pgd[j] = swapper_pg_dir[j];

        // uint64 u_sp = kalloc();
        // create_mapping(task[i]->pgd, USER_END-PGSIZE, u_sp-PA2VA_OFFSET, PGSIZE, EU|EW|ER|EV); // 映射用户栈
        // create_mapping(task[i]->pgd, USER_START, uapp_start-PA2VA_OFFSET, uapp_end-uapp_start, EW|EU|EX|ER|EV); // 映射uapp代码段
        task[i]->mm = (struct mm_struct *)kalloc();
        (task[i]->mm)->mmap = (struct vm_area_struct *)kalloc();
        do_mmap(task[i]->mm, USER_END-PGSIZE, PGSIZE, VM_READ | VM_WRITE);
        do_mmap(task[i]->mm, USER_START, uapp_end-uapp_start, VM_READ | VM_WRITE | VM_EXEC);

        (task[i]->thread).sepc = USER_START;
        //                           SPP   |  SPIE  |  SUM 
        (task[i]->thread).sstatus = (0<<8) | (1<<5) | (1<<18);
        (task[i]->thread).sscratch = USER_END;
    }
    printk("...proc_init done!\n");
}


//dummy
void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            // printk("[PID = %d] is running! thread space begin at = %lx\n", current->pid, current);
        }
    }
}


//switch_to

void switch_to(struct task_struct* next) {
    // printk("%lx %lx\n", current, next);
    if(next == current) return;
    struct task_struct* temp = current;
    current = next;
    __switch_to(temp, next);
}


void do_timer(void) {
    /* 1. 如果当前线程是 idle 线程 或者 当前线程运行剩余时间为0 进行调度 */
    /* 2. 如果当前线程不是 idle 且 运行剩余时间不为0 则对当前线程的运行剩余时间减1 直接返回 */
    if(current == idle || current->counter == 0){
        schedule();
    }
    else current->counter--;
    return;
}

void schedule(void) {
#ifdef SJF
    struct task_struct* next;
    while(1){
        int c = -1;
        next = NULL;
        int i;
        for(i = 1; i <= id; ++i){
            if(((int)task[i]->counter > c) && (task[i]->state == TASK_RUNNING)){
                c = task[i]->counter;
                next = task[i];
            }
        }
        if(c) break;
        for(i = 1; i <= id; ++i){
            task[i]->counter = rand()%10 + 1;
            printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
        }
        printk("\n");
    } 
    printk("switch to [PID = %d COUNTER = %d], pc=%lx\n", next->pid, next->counter, (next->thread).sepc);
    // printk("wow");
    switch_to(next);

#endif

#ifdef PRIORITY

    while(1){
        int c = -1;
        task_struct* next = NULL;
        for(i = 1; i < NR_TASKS; ++i){
            if(task[i]->counter > c && task[i] == TASK_RUNNING){
                c = task[i]->counter;
                next = task[i];
            }
        }
        if(c) break;
        for(i = 1; i < NR_TASKS; ++i){
            task[i]->counter = task[i]->counter/2 + task[i]->priority;
            printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
        }
    } 
    printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n", next->pid, next->priority, next->counter);
    switch_to(next);

#endif

}

struct vm_area_struct *find_vma(struct mm_struct *mm, uint64 addr){
    struct vm_area_struct *p = (mm->mmap)->vm_next;
    while(p != NULL){
        if(addr >= p->vm_start && addr <= p->vm_end) 
            return p;
        p = p->vm_next;
    }
    return NULL;
}

uint64 do_mmap(struct mm_struct *mm, uint64 addr, uint64 length, int prot){
    // 查找该地址是否已经被使用
    struct vm_area_struct *p = (mm->mmap)->vm_next;
    while(p != NULL){
        uint64 l1 = p->vm_start, r1 = p->vm_end-1;
        uint64 l2 = addr, r2 = addr + length - 1;

        uint64 maxl = l1 > l2 ? l1 : l2;
        uint64 minr = r1 < r2 ? r1 : r2;

        if(maxl < minr){ // 相交
            addr = get_unmapped_area(mm, length);
            break;
        } 
        p = p->vm_next;
    }

    // 找到下一个空闲的mm的区域
    struct vm_area_struct* prev = mm->mmap;

    while(prev->vm_next) prev = prev->vm_next;
    p = prev + 1;// 下一个内存区域
    prev->vm_next = p;

    p->vm_mm = mm;
    p->vm_start = addr;
    p->vm_end = addr + length;
    p->vm_next = NULL;
    p->vm_prev = prev;
    p->vm_flags = prot;

    return addr;
}

uint64 get_unmapped_area(struct mm_struct *mm, uint64 length){
    uint64 i;
    struct vm_area_struct *p = (mm->mmap)->vm_next;
    for(i = 0; ; i += PGSIZE){
        int flag = 1;
        while(p != NULL){
            uint64 l1 = p->vm_start, r1 = p->vm_end-1;
            uint64 l2 = i, r2 = i + length - 1;

            uint64 maxl = l1 > l2 ? l1 : l2;
            uint64 minr = r1 < r2 ? r1 : r2;

            if(maxl < minr){ // 相交
                flag = 0;
                break;
            } 
            p = p->vm_next;
        }
        if(flag) return i; 
   }
    return -1;  
}


void forkret() {
    ret_from_fork((struct pt_regs*)current->trapframe);
}

void memcpy(char* x, char* y, int length){
    int i;
    for(i = 0; i < length; ++i, ++x, ++y){
        *x = *y;
    }
}

uint64 do_fork(struct pt_regs *regs){
    // 设置子进程的state, counter, priority, pid
    int i = ++id;
    // printk("i=%lx\n", i);
    task[i] = (struct task_struct *)kalloc();
    task[i]->state = TASK_RUNNING;
    task[i]->counter = 0;//current->counter + 1; //增加counter，使之更可能被调用
    task[i]->priority = rand();
    task[i]->pid = i;
    // printk("%lx\n", &(task[i]->pid));
    // 保存子进程的用户栈*
    (task[i]->thread_info).user_sp = kalloc() + PGSIZE - 8;
    uint64 user_sp;
    asm volatile (
        "csrr t0, sscratch\n"
        "mv %[fork_sp], t0\n"
        : [fork_sp] "=r" (user_sp) : : );
    uint64 user_sp_end = ((user_sp >> 12) << 12) + PGSIZE - 8;
    // printk("user_sp = %lx\n", (task[i]->thread_info).user_sp);

    // printk("%lx, %lx\n", user_sp, user_sp_end);
    char *p = (task[i]->thread_info).user_sp, *q = user_sp_end;
    for(; q >= (char*)user_sp; p--, q--){// 复制用户栈
        *p = *q;  
        // printk("$[%lx]=%x, %lx\n", p, *p, q);
    }

    //设置thread.ra 为forkret
    (task[i]->thread).ra = forkret;
    //设置thread.sp和thread.sscratch
    (task[i]->thread).sp = (char*)task[i] + PGSIZE;
    (task[i]->thread).sscratch = (task[i]->thread).sp;
    //设置thread.sepc
    (task[i]->thread).sepc = regs->sepc;
    //设置sstatus                 SPP   |  SPIE  |  SUM 
    (task[i]->thread).sstatus = (0<<8) | (1<<5) | (1<<18);
    //设置子进程的pgd
    task[i]->pgd = (pagetable_t) kalloc();
    int j;
    for(j = 0; j < 512; ++j)
        task[i]->pgd[j] = swapper_pg_dir[j];
    // printk("pgd = %lx\n", task[i]->pgd);
    // printk("wow = %lx\n", task[i]->pgd[384]);
    //设置子进程的mm，并复制父进程的mm
    task[i]->mm = (struct mm_struct *)kalloc();
    (task[i]->mm)->mmap = (struct vm_area_struct *)kalloc();
    struct vm_area_struct *p_mm = (current->mm->mmap)->vm_next, *c_mm = task[i]->mm->mmap;
    while(p_mm){
        c_mm->vm_next = c_mm+1;
        c_mm++;
        c_mm->vm_mm = task[i]->mm;
        c_mm->vm_start = p_mm->vm_start;
        c_mm->vm_end = p_mm->vm_end;
        c_mm->vm_prev = c_mm-1;
        c_mm->vm_next = NULL;
        c_mm->vm_flags = p_mm->vm_flags;
        p_mm = p_mm->vm_next;
    }
    //保存trapframe
    task[i]->trapframe = kalloc();
    memcpy(task[i]->trapframe, regs, sizeof(struct pt_regs));
    
    task[i]->trapframe->sp = user_sp;
    task[i]->trapframe->a0 = 0;

    // printk("user_sp: %lx\n", user_sp);
    // printk("%lx->%lx\n", task[i], ((task[i]->mm)->mmap));
    return i;
}   
uint64 clone(struct pt_regs *regs) {
    return do_fork(regs);
}