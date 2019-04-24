#include <memlayout.h>

# vectors.S sends all traps here.
.text
.globl __alltraps
__alltraps:
    # push registers to build a trap frame
    # therefore make the stack look like a struct trapframe
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    pushal

    # load GD_KDATA into %ds and %es to set up data segments for kernel
    # 设置数据段为内核数据段
    movl $GD_KDATA, %eax
    movw %ax, %ds
    movw %ax, %es

    # push %esp to pass a pointer to the trapframe as an argument to trap()
    pushl %esp

    # call trap(tf), where tf=%esp
    call trap

    # pop the pushed stack pointer
    popl %esp

    # return falls through to trapret...
.globl __trapret
__trapret:
    # restore registers from stack
    # 恢复现场
    popal

    # restore %ds, %es, %fs and %gs
    # 恢复段寄存器
    popl %gs
    popl %fs
    popl %es
    popl %ds

    # get rid of the trap number and error code
    # 弹出中断服务程序压入的中断编号和错误码
    addl $0x8, %esp
    iret

