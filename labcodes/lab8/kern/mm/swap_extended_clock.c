#include <defs.h>
#include <list.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_extended_clock.h>
#include <x86.h>

static int _clock_init_mm(struct mm_struct *mm) {
  list_init(&mm->page_list);
  // 初始化 sm_priv 队列
  mm->sm_priv = &mm->page_list;
  return 0;
}

static int _clock_map_swappable(struct mm_struct *mm, uintptr_t addr,
                                struct Page *page, int swap_in) {
  list_entry_t *head = (list_entry_t *)mm->sm_priv;
  list_entry_t *entry = &(page->pra_page_link);
  list_add_before(head, entry);
  return 0;
}

static int _clock_swap_out_victim(struct mm_struct *mm, struct Page **ptr_page,
                                  int in_tick) {
  struct proc_struct *least = NULL;
  list_entry_t *le;
  // 找出页错误最少的 proc_struct
  for (le = list_next(&proc_list); le != &proc_list;
       le = list_next(le)) {
    struct proc_struct *proc = le2proc(le, list_link);
    if (!proc->mm || list_empty((list_entry_t *)proc->mm->sm_priv)) continue;
    if (!least || proc->page_fault < least->page_fault) least = proc;
  }
  if (least) mm = least->mm;

  list_entry_t *head = (list_entry_t *)mm->sm_priv;  // 获取 sm_priv 队列指针
  struct Page *victim = NULL;  // 定义被换出的内存页

  // 查找未被引用也未被修改的内存页
  for (le = list_next(head); le != head; le = list_next(le)) {
    struct Page *page = le2page(le, pra_page_link);  // 获取调出页的物理地址
    pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);  // 获取页表项
    if (!(*ptep & PTE_A) && !(*ptep & PTE_D)) {
      victim = page;
      break;
    }
  }
  if (victim) goto found;

  // 查找未被引用但被修改的内存页
  for (le = list_next(head); le != head; le = list_next(le)) {
    struct Page *page = le2page(le, pra_page_link);  // 获取调出页的物理地址
    pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);  // 获取页表项

    if (!(*ptep & PTE_A) && (*ptep & PTE_D)) {
      victim = page;
      break;
    }
    *ptep &= ~PTE_A;
    tlb_invalidate(mm->pgdir, page->pra_vaddr);  // 更新快表
  }
  if (victim) goto found;

  // 查找被引用但未被修改的内存页
  for (le = list_next(head); le != head; le = list_next(le)) {
    struct Page *page = le2page(le, pra_page_link);  // 获取调出页的物理地址
    pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);  // 获取页表项
    if ((*ptep & PTE_A) && !(*ptep & PTE_D)) {
      victim = page;
      break;
    }
  }
  if (victim) goto found;

  // 查找未被引用也未被修改的内存页
  for (le = list_next(head); le != head; le = list_next(le)) {
    struct Page *page = le2page(le, pra_page_link);  // 获取调出页的物理地址
    pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);  // 获取页表项
    if (!(*ptep & PTE_A) && (*ptep & PTE_D)) {
      victim = page;
      break;
    }
    *ptep &= ~PTE_A;
    tlb_invalidate(mm->pgdir, page->pra_vaddr);  // 更新快表
  }
  if (victim) goto found;

found:
  list_del(&victim->pra_page_link);  //将此页从 pra_list_head 队列移除
  *ptr_page = victim;
  return 0;
}

static int _clock_check_swap(void) { return 0; }

static int _clock_init(void) { return 0; }

static int _clock_set_unswappable(struct mm_struct *mm, uintptr_t addr) {
  return 0;
}

static int _clock_tick_event(struct mm_struct *mm) { return 0; }

struct swap_manager swap_manager_extended_clock = {
    .name = "extended clock swap manager",
    .init = &_clock_init,
    .init_mm = &_clock_init_mm,
    .tick_event = &_clock_tick_event,
    .map_swappable = &_clock_map_swappable,
    .set_unswappable = &_clock_set_unswappable,
    .swap_out_victim = &_clock_swap_out_victim,
    .check_swap = &_clock_check_swap,
};