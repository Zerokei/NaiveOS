.extern start_kernel
.extern mm_init
.extern _traps
.extern early_pgtbl
.extern setup_vm
.extern setup_vm_final
    .section .text.init
    .globl _start
_start:
	# ------------------
    call setup_vm
    call relocate

    la sp, boot_stack_top

    # set stvec = _traps
    la t0, _traps # store address in t0
    la t1, -4
    and t0, t0, t1# set Direct mode
    csrw stvec, t0
    # set sie[STIE] = 1
    la t0, 0x00000020
    csrs sie, t0
    # set first time interrupt
    rdtime t0
    la t1, 10000000 # 1e7 is too large
    add t1, t1, t0
    mv a0, t1
    mv a1, x0
    mv a2, x0
    mv a3, x0
    mv a4, x0
    mv a5, x0
    mv a6, x0
    mv a7, x0
    ecall

    # 禁用 sstatus[SIE]
    # # set sstatus[SIE] = 1
    # la t0, 0x00000002
    # csrs sstatus, t0
    
    # init
    call mm_init
    call setup_vm_final
    call task_init
    # call start_kernel
    call start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    
    #a0 = 0xFFFFFFDF80000000
    li a0, 0xFFFFFFDF80000000

    add ra, ra, a0
    add sp, sp, a0

    # set satp with early_pgtbl
    li a1, 0x8000000000000000
    la a0, early_pgtbl
    srl a0, a0, 12
    add a0, a0, a1
    csrw satp, a0

    # flush tlb
    sfence.vma zero, zero

    ret
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top:
