#include <assert.h>
#include <list.h>
#include <proc.h>
#include <sched.h>
#include <sync.h>

void wakeup_proc(struct proc_struct *proc) {
  assert(proc->state != PROC_ZOMBIE && proc->state != PROC_RUNNABLE);
  proc->state = PROC_RUNNABLE;
}

void schedule(void) {
  bool intr_flag;
  list_entry_t *le, *last;
  struct proc_struct *next = NULL;
  local_intr_save(intr_flag);  // 关闭中断
  {
    current->need_resched = 0;
    last = (current == idleproc) ? &proc_list : &(current->list_link);
    le = last;
    // 查询就绪态进程
    do {
      if ((le = list_next(le)) != &proc_list) {
        next = le2proc(le, list_link);
        if (next->state == PROC_RUNNABLE) {
          break;
        }
      }
    } while (le != last);
    if (next == NULL || next->state != PROC_RUNNABLE) {
      next = idleproc;
    }
    next->runs++;
    if (next != current) {
      proc_run(next);  // 执行已查询到的一个就绪态进程
    }
  }
  local_intr_restore(intr_flag);  // 开启中断
}
