#include <assert.h>
#include <atomic.h>
#include <defs.h>
#include <kmalloc.h>
#include <proc.h>
#include <sem.h>
#include <sync.h>
#include <wait.h>

void sem_init(semaphore_t *sem, int value) {
  sem->value = value;
  wait_queue_init(&(sem->wait_queue));
}

static __noinline void __up(semaphore_t *sem, uint32_t wait_state) {
  bool intr_flag;
  local_intr_save(intr_flag);  // 关闭中断
  {
    wait_t *wait;
    if ((wait = wait_queue_first(&(sem->wait_queue))) == NULL) {
      sem->value++;
    } else {
      assert(wait->proc->wait_state == wait_state);
      // 唤醒等待队列中的一个进程
      wakeup_wait(&(sem->wait_queue), wait, wait_state, 1);
    }
  }
  local_intr_restore(intr_flag);  // 开启中断
}

static __noinline uint32_t __down(semaphore_t *sem, uint32_t wait_state) {
  bool intr_flag;
  local_intr_save(intr_flag);  // 关闭中断
  if (sem->value > 0) {
    sem->value--;                   // 将信号量值减一
    local_intr_restore(intr_flag);  // 开启中断
    return 0;
  }
  wait_t __wait, *wait = &__wait;
  // 若信号量值小于 0 ，则将当前进程加入至等待队列
  wait_current_set(&(sem->wait_queue), wait, wait_state);
  local_intr_restore(intr_flag);  // 开启中断

  schedule();  // 调度

  local_intr_save(intr_flag);  // 关闭中断
  // 调度访问后，将当前进程从等待队列删除
  wait_current_del(&(sem->wait_queue), wait);
  local_intr_restore(intr_flag);  // 开启中断

  if (wait->wakeup_flags != wait_state) {
    return wait->wakeup_flags;
  }
  return 0;
}

// V 操作
void up(semaphore_t *sem) { __up(sem, WT_KSEM); }

// P 操作
void down(semaphore_t *sem) {
  uint32_t flags = __down(sem, WT_KSEM);
  assert(flags == 0);
}

bool try_down(semaphore_t *sem) {
  bool intr_flag, ret = 0;
  local_intr_save(intr_flag);
  if (sem->value > 0) {
    sem->value--, ret = 1;
  }
  local_intr_restore(intr_flag);
  return ret;
}
