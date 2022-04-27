#include "types.h"
#include "sbi.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
    struct sbiret r;
    uint64 a, b, e = ext, f = fid;
    __asm__ volatile(
        "mv t3, %[arg0]\n"
        "mv t4, %[arg1]\n"
        "mv a2, %[arg2]\n"
        "mv a3, %[arg3]\n"
        "mv a4, %[arg4]\n"
        "mv a5, %[arg5]\n"
        "mv a6, %[f]\n"
        "mv a7, %[e]\n"
        "mv a0, t3\n"
        "mv a1, t4\n"
        "ecall\n"
        "mv %[a], a0\n"
        "mv %[b], a1\n"
        : [a] "=r" (a), [b] "=r" (b)
        : [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2), [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5), [e] "r" (e), [f] "r" (f)
        : "memory"
    );
    r.error=a;
    r.value=b;
    return r;
}
