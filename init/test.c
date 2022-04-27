#include "printk.h"
#include "defs.h"

// Please do not modify

void test() {
    int pas = 0;
    while (1){
        int clock;
        __asm__ volatile("rdtime %[clock]\n": [clock] "=r" (clock) : : "memory");
        if(clock/10000000 > pas){
            pas = clock/10000000;
            printk("kernel is running!\n");
        }
    }
}
