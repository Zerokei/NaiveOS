// arch/riscv/kernel/vm.c
#include "defs.h"
#include "mm.h"
#include "string.h"
/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

#define EV 1
#define ER 2
#define EW 4
#define EX 8

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */

    int i = ((unsigned long)(PHY_START)>>30)&0x1ff;
    early_pgtbl[i] = (((unsigned long)(PHY_START)>>30)&0xfff)<<28;
    early_pgtbl[i] = early_pgtbl[i] | EV | ER | EW | EX;
    
    int j = ((unsigned long)(VM_START)>>30)&0x1ff;  
    early_pgtbl[j] = (((unsigned long)(PHY_START)>>30)&0xfff)<<28;
    early_pgtbl[j] = early_pgtbl[i] | EV | ER | EW | EX;
}

/* 创建多级页表映射关系 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
     
    uint64 vp, pp = pa;
    uint64 ed = va + sz;
    // printk("sz=%lx va=%lx, ed=%lx\n", sz, va, ed);
    for(vp = va; vp <= ed; vp += PGSIZE, pp += PGSIZE){
        int i;
        uint64* prepage;
        uint64 p2,p3;
        
        i = (vp>>30)&0x1ff;
        // printk("sz=%lx vp=%lx\n", sz, vp);
        prepage = (uint64*)pgtbl + i; 
        if((*prepage & 1) == 0){
            p2 = kalloc();
            *prepage = (((p2-PA2VA_OFFSET)>>12)<<10)|1;
        }else p2 = ((*prepage)>>10)<<12;
        // printk("%lx->%lx\n", prepage, p2);

        i = (vp>>21)&0x1ff;
        prepage = (uint64*)p2 + i;
        if((*prepage & 1) == 0){
            p3 = kalloc();
            *prepage = (((p3-PA2VA_OFFSET)>>12)<<10)|1;
        }else p3 = ((*prepage)>>10)<<12;
        // printk("%lx->%lx\n", prepage, p3);

        i = (vp>>12)&0x1ff;
        prepage = (uint64*)p3 + i;
        // printk("%lx->%lx\n", prepage, (((uint64)pp>>12))<<10|perm);
        *prepage = (((uint64)pp>>12))<<10|perm;
    }
}
extern char _skernel[];
extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];
extern char _ebss[];

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, _stext, _stext-PA2VA_OFFSET, _etext-_stext, EX|ER|EV);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, _srodata, _srodata-PA2VA_OFFSET, _erodata-_srodata, ER|EV);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, _sdata, _sdata-PA2VA_OFFSET, VM_START+PHY_SIZE-(uint64)_sdata, EW|ER|EV);

    // set satp with swapper_pg_dir

    unsigned long* p = swapper_pg_dir;
    uint64 arg0 = ((uint64)p-PA2VA_OFFSET) >> 12;
    arg0 &= 0xfffffffffff;
    // arg0 >>= 12;
    __asm__ volatile(
        "li t3, 0x8000000000000000\n"
        "mv t4, %[arg0]\n"
        "add t3, t3, t4\n"
        "csrw satp, t3\n"
        : : [arg0] "r" (arg0) : "memory"
    );

    asm volatile("sfence.vma zero, zero");
    arg0 = (uint64)_stext;
    __asm__ volatile(
        "mv t4, %[arg0]\n"
        "sd x0, 0(t4)\n"
        : : [arg0] "r" (arg0) : "memory"
    );
    // flush TLB
    return;
}

