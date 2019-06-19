#include <assert.h>
#include <default_sched.h>
#include <defs.h>
#include <list.h>
#include <proc.h>

#define USE_SKEW_HEAP 1

/* You should define the BigStride constant here*/
/* LAB6: YOUR CODE */
#define BIG_STRIDE 0x7FFFFFFF /* ??? */

/* The compare function for two skew_heap_node_t's and the
 * corresponding procs*/
static int proc_stride_comp_f(void *a, void *b) {
  struct proc_struct *p = le2proc(a, lab6_run_pool);
  struct proc_struct *q = le2proc(b, lab6_run_pool);
  int32_t c = p->lab6_stride - q->lab6_stride;
  if (c > 0)
    return 1;
  else if (c == 0)
    return 0;
  else
    return -1;
}

/*
 * stride_init initializes the run-queue rq with correct assignment for
 * member variables, including:
 *
 *   - run_list: should be a empty list after initialization.
 *   - lab6_run_pool: NULL
 *   - proc_num: 0
 *   - max_time_slice: no need here, the variable would be assigned by the
 * caller.
 *
 * hint: see proj13.1/libs/list.h for routines of the list structures.
 */
static void stride_init(struct run_queue *rq) {
  list_init(&(rq->run_list));
  rq->lab6_run_pool = NULL;
  rq->proc_num = 0;
}

/*
 * stride_enqueue inserts the process ``proc'' into the run-queue
 * ``rq''. The procedure should verify/initialize the relevant members
 * of ``proc'', and then put the ``lab6_run_pool'' node into the
 * queue(since we use priority queue here). The procedure should also
 * update the meta date in ``rq'' structure.
 *
 * proc->time_slice denotes the time slices allocation for the
 * process, which should set to rq->max_time_slice.
 *
 * hint: see proj13.1/libs/skew_heap.h for routines of the priority
 * queue structures.
 */
static void stride_enqueue(struct run_queue *rq, struct proc_struct *proc) {
  // 将当前进程插入斜堆
  rq->lab6_run_pool = skew_heap_insert(
      rq->lab6_run_pool, &(proc->lab6_run_pool), proc_stride_comp_f);
  // 计算当前进程的剩余时间片
  if (proc->time_slice == 0 || proc->time_slice > rq->max_time_slice) {
    proc->time_slice = rq->max_time_slice;
  }
  proc->rq = rq;
  rq->proc_num++;  // 递增就绪斜堆大小
}

/*
 * stride_dequeue removes the process ``proc'' from the run-queue
 * ``rq'', the operation would be finished by the skew_heap_remove
 * operations. Remember to update the ``rq'' structure.
 *
 * hint: see proj13.1/libs/skew_heap.h for routines of the priority
 * queue structures.
 */
static void stride_dequeue(struct run_queue *rq, struct proc_struct *proc) {
  // 将当前进程从斜堆移除
  rq->lab6_run_pool = skew_heap_remove(
      rq->lab6_run_pool, &(proc->lab6_run_pool), proc_stride_comp_f);
  rq->proc_num--;  // 递减就绪斜堆大小
}
/*
 * stride_pick_next pick the element from the ``run-queue'', with the
 * minimum value of stride, and returns the corresponding process
 * pointer. The process pointer would be calculated by macro le2proc,
 * see proj13.1/kern/process/proc.h for definition. Return NULL if
 * there is no process in the queue.
 *
 * When one proc structure is selected, remember to update the stride
 * property of the proc. (stride += BIG_STRIDE / priority)
 *
 * hint: see proj13.1/libs/skew_heap.h for routines of the priority
 * queue structures.
 */
static struct proc_struct *stride_pick_next(struct run_queue *rq) {
  if (rq->lab6_run_pool == NULL) {
    return NULL;
  }
  // 找到 stride 值最小的进程
  struct proc_struct *p = le2proc(rq->lab6_run_pool, lab6_run_pool);
  // 增加 stride 的值
  if (p->lab6_priority == 0) {
    p->lab6_stride += BIG_STRIDE;
  } else {
    p->lab6_stride += BIG_STRIDE / p->lab6_priority;
  }
  return p;
}

/*
 * stride_proc_tick works with the tick event of current process. You
 * should check whether the time slices for current process is
 * exhausted and update the proc struct ``proc''. proc->time_slice
 * denotes the time slices left for current
 * process. proc->need_resched is the flag variable for process
 * switching.
 */
static void stride_proc_tick(struct run_queue *rq, struct proc_struct *proc) {
  if (proc->time_slice > 0) {  // 若当前进程还有时间片，则时间片递减
    proc->time_slice--;
  }
  if (proc->time_slice == 0) {  // 若当前进程没有时间片，则需要重新调度
    proc->need_resched = 1;
  }
}

struct sched_class default_sched_class = {
    .name = "stride_scheduler",     // 设置 sched_class 名称
    .init = stride_init,            // 设置 init 函数指针
    .enqueue = stride_enqueue,      // 设置 enqueue 函数指针
    .dequeue = stride_dequeue,      // 设置 dequeue 函数指针
    .pick_next = stride_pick_next,  // 设置 pick_next 函数指针
    .proc_tick = stride_proc_tick,  // 设置 proc_tick 函数指针
};
