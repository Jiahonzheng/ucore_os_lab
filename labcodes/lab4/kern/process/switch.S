.text
.globl switch_to
switch_to:                      # switch_to(from, to)

    # save from's registers
    # 保存当前进程的通用寄存器内容
    movl 4(%esp), %eax          # eax points to from
    popl 0(%eax)                # save eip !popl
    movl %esp, 4(%eax)          # 保存esp的值到context的esp
    movl %ebx, 8(%eax)          # 保存ebx的值到context的ebx
    movl %ecx, 12(%eax)         # 保存ecx的值到context的ecx
    movl %edx, 16(%eax)         # 保存edx的值到context的edx
    movl %esi, 20(%eax)         # 保存esi的值到context的esi
    movl %edi, 24(%eax)         # 保存edi的值到context的edi
    movl %ebp, 28(%eax)         # 保存ebq的值到context的ebp

    # restore to's registers
    # 恢复目的进程的通用寄存器内容
    movl 4(%esp), %eax          # not 8(%esp): popped return address already
                                # eax now points to to
    movl 28(%eax), %ebp         # 保存context的ebp到ebp
    movl 24(%eax), %edi         # 保存context的edi到edi
    movl 20(%eax), %esi         # 保存context的esi到esi
    movl 16(%eax), %edx         # 保存context的edx到edx
    movl 12(%eax), %ecx         # 保存context的ecx到ecx
    movl 8(%eax), %ebx          # 保存context的ebx到ebx
    movl 4(%eax), %esp          # 保存context的esp到esp

    pushl 0(%eax)               # push eip

    ret

