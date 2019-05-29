#ifndef __KERN_PROCESS_PROC_H__
#define __KERN_PROCESS_PROC_H__

#include <defs.h>
#include <list.h>
#include <memlayout.h>
#include <trap.h>

// process's state in his life cycle
enum proc_state {
  PROC_UNINIT = 0,  // 未初始化
  PROC_SLEEPING,    // 阻塞
  PROC_RUNNABLE,    // 就绪
  PROC_ZOMBIE,      // 僵死
};

// Saved registers for kernel context switches.
// Don't need to save all the %fs etc. segment registers,
// because they are constant across kernel contexts.
// Save all the regular registers so we don't need to care
// which are caller save, but not the return register %eax.
// (Not saving %eax just simplifies the switching code.)
// The layout of context must match code in switch.S.
struct context {
  uint32_t eip;  // 指令地址
  uint32_t esp;  // 栈指针
  uint32_t ebx;  // 基址寄存器
  uint32_t ecx;  // 计数
  uint32_t edx;  // 存访数据
  uint32_t esi;  // 源变址
  uint32_t edi;  // 目的变址
  uint32_t ebp;  // 基址指针
};

#define PROC_NAME_LEN 15
#define MAX_PROCESS 4096
#define MAX_PID (MAX_PROCESS * 2)

extern list_entry_t proc_list;

struct proc_struct {
  enum proc_state state;         // 进程状态
  int pid;                       // 进程唯一标识
  int runs;                      // 进程运行次数
  uintptr_t kstack;              // 进程内核栈
  volatile bool need_resched;    // 当前进程是否需要调度
  struct proc_struct *parent;    // 父进程
  struct mm_struct *mm;          // 进程内存管理块
  struct context context;        // 进程上下文
  struct trapframe *tf;          // 当前中断陷入帧
  uintptr_t cr3;                 // 页目录表基址
  uint32_t flags;                // 进程标记位
  char name[PROC_NAME_LEN + 1];  // 进程名称
  list_entry_t list_link;        // 进程控制块链表
  list_entry_t hash_link;  // 进程控制块链表，利用 hash 加快寻找速度
};

#define le2proc(le, member) to_struct((le), struct proc_struct, member)

extern struct proc_struct *idleproc, *initproc, *current;

void proc_init(void);
void proc_run(struct proc_struct *proc);
int kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags);

char *set_proc_name(struct proc_struct *proc, const char *name);
char *get_proc_name(struct proc_struct *proc);
void cpu_idle(void) __attribute__((noreturn));

struct proc_struct *find_proc(int pid);
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);
int do_exit(int error_code);

#endif /* !__KERN_PROCESS_PROC_H__ */
