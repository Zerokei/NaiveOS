typedef unsigned long uint64;
#define SYS_WRITE   64
#define SYS_GETPID  172

#define SYS_MUNMAP   215
#define SYS_CLONE    220 // fork
#define SYS_MMAP     222
#define SYS_MPROTECT 226
struct pt_regs{
    uint64 zero;
    uint64 ra;
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0;
    uint64 s1;
    uint64 a0;
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;
    uint64 sepc;
};

void deal_syscall(struct pt_regs* regs);
uint64 do_fork(struct pt_regs *regs);