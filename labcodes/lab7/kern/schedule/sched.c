#include <assert.h>
#include <default_sched.h>
#include <list.h>
#include <proc.h>
#include <sched.h>
#include <stdio.h>
#include <sync.h>

// the list of timer
static list_entry_t timer_list;

static struct sched_class *sched_class;

static struct run_queue *rq;

static inline void sched_class_enqueue(struct proc_struct *proc) {
  if (proc != idleproc) {
    sched_class->enqueue(rq, proc);
  }
}

static inline void sched_class_dequeue(struct proc_struct *proc) {
  sched_class->dequeue(rq, proc);
}

static inline struct proc_struct *sched_class_pick_next(void) {
  return sched_class->pick_next(rq);
}

static void sched_class_proc_tick(struct proc_struct *proc) {
  if (proc != idleproc) {
    sched_class->proc_tick(rq, proc);
  } else {
    proc->need_resched = 1;
  }
}

static struct run_queue __rq;

void sched_init(void) {
  list_init(&timer_list);

  sched_class = &default_sched_class;

  rq = &__rq;
  rq->max_time_slice = 5;
  sched_class->init(rq);

  cprintf("sched class: %s\n", sched_class->name);
}

void wakeup_proc(struct proc_struct *proc) {
  assert(proc->state != PROC_ZOMBIE);
  bool intr_flag;
  local_intr_save(intr_flag);
  {
    if (proc->state != PROC_RUNNABLE) {
      proc->state = PROC_RUNNABLE;
      proc->wait_state = 0;
      if (proc != current) {
        sched_class_enqueue(proc);
      }
    } else {
      warn("wakeup runnable process.\n");
    }
  }
  local_intr_restore(intr_flag);
}

void schedule(void) {
  bool intr_flag;
  struct proc_struct *next;
  local_intr_save(intr_flag);
  {
    current->need_resched = 0;
    if (current->state == PROC_RUNNABLE) {
      sched_class_enqueue(current);
    }
    if ((next = sched_class_pick_next()) != NULL) {
      sched_class_dequeue(next);
    }
    if (next == NULL) {
      next = idleproc;
    }
    next->runs++;
    if (next != current) {
      proc_run(next);
    }
  }
  local_intr_restore(intr_flag);
}

// 增加计时器至全局计时器链表
void add_timer(timer_t *timer) {
  bool intr_flag;
  local_intr_save(intr_flag);  // 关闭中断
  {
    assert(timer->expires > 0 && timer->proc != NULL);
    assert(list_empty(&(timer->timer_link)));
    list_entry_t *le = list_next(&timer_list);
    while (le != &timer_list) {
      timer_t *next = le2timer(le, timer_link);
      // 更新计时器的过期时间
      if (timer->expires < next->expires) {
        next->expires -= timer->expires;
        break;
      }
      timer->expires -= next->expires;
      le = list_next(le);
    }
    list_add_before(le, &(timer->timer_link));  // 添加定时器至链表
  }
  local_intr_restore(intr_flag);  // 开启中断
}

// 从定时器链表删除定时器
void del_timer(timer_t *timer) {
  bool intr_flag;
  local_intr_save(intr_flag);  // 关闭中断
  {
    if (!list_empty(&(timer->timer_link))) {
      if (timer->expires != 0) {
        list_entry_t *le = list_next(&(timer->timer_link));
        // 更新定时器的过期时间
        if (le != &timer_list) {
          timer_t *next = le2timer(le, timer_link);
          next->expires += timer->expires;
        }
      }
      list_del_init(&(timer->timer_link));  // 从定时器链表删除定时器
    }
  }
  local_intr_restore(intr_flag);  // 开启中断
}

// call scheduler to update tick related info, and check the timer is expired?
// If expired, then wakup proc
// 执行定时器
void run_timer_list(void) {
  bool intr_flag;
  local_intr_save(intr_flag);  // 关闭中断
  {
    list_entry_t *le = list_next(&timer_list);
    if (le != &timer_list) {
      timer_t *timer = le2timer(le, timer_link);
      assert(timer->expires != 0);
      timer->expires--;
      // 唤醒所有已过期计时器绑定的进程
      while (timer->expires == 0) {
        le = list_next(le);
        struct proc_struct *proc = timer->proc;
        if (proc->wait_state != 0) {
          assert(proc->wait_state & WT_INTERRUPTED);
        } else {
          warn("process %d's wait_state == 0.\n", proc->pid);
        }
        wakeup_proc(proc);  // 唤醒计时器绑定的进程
        del_timer(timer);   // 删除计时器
        if (le == &timer_list) {
          break;
        }
        timer = le2timer(le, timer_link);
      }
    }
    sched_class_proc_tick(current);  // 调度
  }
  local_intr_restore(intr_flag);  // 开启中断
}
