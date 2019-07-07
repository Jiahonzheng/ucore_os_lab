#include <assert.h>
#include <default_sched_MLFQ.h>
#include <defs.h>
#include <list.h>
#include <proc.h>
#include <stdio.h>

#define QUEUE_NUM 6

// 调度队列
static list_entry_t run_list[QUEUE_NUM];
// 多级调度队列每级对应的时间片比例系数
static int slice_factor[QUEUE_NUM];

static void MLFQ_init(struct run_queue *rq) {
  int i;
  for (i = 0; i < QUEUE_NUM; ++i) {
    list_init(&run_list[i]);  // 初始化多级调度队列
    if (i == 0) {
      slice_factor[i] = 1;  // 最高级时间片
    } else {
      // 每降低 1 级，其时间片为上一级时间片乘 2
      slice_factor[i] = slice_factor[i - 1] * 2;
    }
  }
  rq->proc_num = 0;  // 设置就绪队列进程数目为 0
}

// 将进程加入到就绪队列中
static void MLFQ_enqueue(struct run_queue *rq, struct proc_struct *proc) {
  // 将进程加入到调度队列中
  list_add_before(&run_list[proc->lab6_stride], &(proc->run_link));
  // 如果进程的时间片已到，那么重置该进程的时间片
  int max_time_slice = rq->max_time_slice * slice_factor[proc->lab6_stride];
  if (proc->time_slice == 0 || proc->time_slice > max_time_slice) {
    proc->time_slice = max_time_slice;
  }
  proc->rq = rq;
  rq->proc_num++;  // 增加就绪队列中的进程数
  // 用于测试
  cprintf("Process %d 's priority is %d.\n", proc->pid, proc->lab6_stride);
}

// 将进程从就绪队列中删除
static void MLFQ_dequeue(struct run_queue *rq, struct proc_struct *proc) {
  list_del_init(&(proc->run_link));  // 将进程从就绪队列中删除
  rq->proc_num--;                    // 减少就绪队列的进程数
  cprintf("Process %d is running.\n", proc->pid);  // 用于测试
}

// 选择下一个要执行的进程
static struct proc_struct *MLFQ_pick_next(struct run_queue *rq) {
  int i;
  for (i = 0; i < QUEUE_NUM; ++i) {
    if (!list_empty(&run_list[i])) {
      // 从调度队列队头获取一进程，因为它是最长时间内未获得执行权的进程
      list_entry_t *le = list_next(&run_list[i]);
      struct proc_struct *p = le2proc(le, run_link);
      if (p->lab6_stride + 1 < QUEUE_NUM) {
        p->lab6_stride++;
        // 用于测试
        cprintf("Process %d 's priority is promoted to %d from %d.\n", p->pid,
                p->lab6_stride, p->lab6_stride - 1);
      }
      return p;
    }
  }
  return NULL;
}

// 时间中断调度处理函数
static void MLFQ_proc_tick(struct run_queue *rq, struct proc_struct *proc) {
  if (proc->time_slice > 0) {
    // 时间片未到期，减少时间片
    proc->time_slice--;
  } else if (proc->time_slice == 0) {
    // 时间片到期，设置进程状态为需要调度
    proc->need_resched = 1;
  }
}

struct sched_class MLFQ_sched_class = {
    .name = "MLFQ_Scheduler",     // 设置 sched_class 名称
    .init = MLFQ_init,            // 设置 init 函数指针
    .enqueue = MLFQ_enqueue,      // 设置 enqueue 函数指针
    .dequeue = MLFQ_dequeue,      // 设置 dequeue 函数指针
    .pick_next = MLFQ_pick_next,  // 设置 pick_next 函数指针
    .proc_tick = MLFQ_proc_tick,  // 设置 proc_tick 函数指针
};
