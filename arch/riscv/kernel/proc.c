//arch/riscv/kernel/proc.c
#include<proc.h>

extern void __dummy();

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组，所有的线程都保存在此

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
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`, 
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址， `sp` 设置为 该线程申请的物理页的高地址

    int i;
    for(i = 1; i < NR_TASKS; ++i){
        task[i] = (struct task_struct *)kalloc();
        task[i]->pid = i;
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        (task[i]->thread).ra = (uint64)__dummy;
        (task[i]->thread).sp = (uint64)task[i] + PGSIZE; //设置sp
    }
    printk("...proc_init done!\n");
}


//dummy
void dummy() {
    // printk("dummy!!!\n");
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            // printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            printk("[PID = %d] is running! thread space begin at = %lx\n", current->pid, current);
            // printk("counter = %x\n", current->counter); 
        }
    }
}


//switch_to
extern void __switch_to(struct task_struct* prev, struct task_struct* next);

void switch_to(struct task_struct* next) {
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
        for(i = 1; i < NR_TASKS; ++i){
            if(((int)task[i]->counter > c) && (task[i]->state == TASK_RUNNING)){
                c = task[i]->counter;
                next = task[i];
            }
        }
        if(c) break;
        for(i = 1; i < NR_TASKS; ++i){
            task[i]->counter = rand()%3;
            printk("SET [PID = %d COUNTER = %d]\n", task[i]->pid, task[i]->counter);
        }
    } 
    printk("switch to [PID = %d COUNTER = %d]\n", next->pid, next->counter);
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