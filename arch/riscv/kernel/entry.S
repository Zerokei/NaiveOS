.extern trap_handler
    .section .text.entry
    .align 2
    .globl _traps
_traps:
    # YOUR CODE HERE
    # 1. save 32 registers and sepc to stack
    csrr tp, sscratch
	beqz tp, _save_context
_save_user_sp:
    csrw sscratch, sp
	mv sp, tp
_save_context: 
    sd x2, 16-256(sp)
    addi sp, sp, -256
    sd x0, 0(sp)
    sd x1, 8(sp)
    # sd x2, 16(sp)
    sd x3, 24(sp)
    sd x4, 32(sp)
    sd x5, 40(sp)
    sd x6, 48(sp)
    sd x7, 56(sp)
    sd x8, 64(sp)
    sd x9, 72(sp)
    sd x10, 80(sp)
    sd x11, 88(sp)
    sd x12, 96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)
    csrr t0, sepc
    sd t0, 256(sp)
    # 保存 sepc和sscratch
    # 2. call trap_handler
    csrr a0, scause
    csrr a1, sepc
    mv a2, sp # 保存pt_regs *regs

    call trap_handler
    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack

    # 恢复sscratch 和 sepc
    ld x0, 0(sp)
    ld x1, 8(sp)
    ld x3, 24(sp)
    ld x4, 32(sp)
    ld x5, 40(sp)
    ld x6, 48(sp)
    ld x7, 56(sp)
    ld x8, 64(sp)
    ld x9, 72(sp)
    ld x10, 80(sp)
    ld x11, 88(sp)
    ld x12, 96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)
    ld x2, 16(sp)

    csrr tp, sscratch
	beqz tp, _go_on
_restore_user_sp:
    csrw sscratch, sp
	mv sp, tp
_go_on:
    # 4. return from trap
    sret
    # -----------
.extern dummy
    .global __dummy

# .extern uapp_start
__dummy:
    csrr t0, sscratch
    csrw sscratch, sp # 保存S-mode的sp
    mv sp, t0 # 写入U-mode的sp    
    sret


    .globl __switch_to
__switch_to:
    # save state to prev process
    sd ra, 48(a0)
    sd sp, 56(a0)
    sd s0, 64(a0)
    sd s1, 72(a0)
    sd s2, 80(a0)
    sd s3, 88(a0)
    sd s4, 96(a0)
    sd s5, 104(a0)
    sd s6, 112(a0)
    sd s7, 120(a0)
    sd s8, 128(a0)
    sd s9, 136(a0)
    sd s10, 144(a0)
    sd s11, 152(a0)
    csrr a2, sepc
    sd a2, 160(a0)
    csrr a2, sstatus
    sd a2, 168(a0)
    csrr a2, sscratch
    sd a2, 176(a0)
    
    # restore state from next process

    ld ra, 48(a1)
    ld sp, 56(a1)
    ld s0, 64(a1)
    ld s1, 72(a1)
    ld s2, 80(a1)
    ld s3, 88(a1)
    ld s4, 96(a1)
    ld s5, 104(a1)
    ld s6, 112(a1)
    ld s7, 120(a1)
    ld s8, 128(a1)
    ld s9, 136(a1)
    ld s10, 144(a1)
    ld s11, 152(a1)
    ld a2, 160(a1)
    csrw sepc, a2
    ld a2, 168(a1)
    csrw sstatus, a2
    ld a2, 176(a1)
    csrw sscratch, a2

    # 切换页表
    ld a2, 184(a1)
    li a1, 0xFFFFFFDF80000000
    sub a2, a2, a1 #减去PA2VA_OFFSET
    srl a2, a2, 12
    li a1, 0x8000000000000000
    add a2, a2, a1
    csrw satp, a2
    sfence.vma zero, zero
    ret

    .globl ret_from_fork
ret_from_fork:
    csrw sscratch, sp

    csrr t0, sepc
    addi t0, t0, 4
    csrw sepc, t0
    mv t0, a0
    ld x0, 0(t0)
    ld x1, 8(t0)
    ld x2, 16(t0)
    ld x3, 24(t0)
    ld x4, 32(t0)
    ld x6, 48(t0)
    ld x7, 56(t0)
    ld x8, 64(t0)
    ld x9, 72(t0)
    ld x10, 80(t0)
    ld x11, 88(t0)
    ld x12, 96(t0)
    ld x13, 104(t0)
    ld x14, 112(t0)
    ld x15, 120(t0)
    ld x16, 128(t0)
    ld x17, 136(t0)
    ld x18, 144(t0)
    ld x19, 152(t0)
    ld x20, 160(t0)
    ld x21, 168(t0)
    ld x22, 176(t0)
    ld x23, 184(t0)
    ld x24, 192(t0)
    ld x25, 200(t0)
    ld x26, 208(t0)
    ld x27, 216(t0)
    ld x28, 224(t0)
    ld x29, 232(t0)
    ld x30, 240(t0)
    ld x31, 248(t0)
    ld x5, 40(t0)
    sret 
_end: