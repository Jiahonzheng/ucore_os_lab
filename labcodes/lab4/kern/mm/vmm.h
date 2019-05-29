#ifndef __KERN_MM_VMM_H__
#define __KERN_MM_VMM_H__

#include <defs.h>
#include <list.h>
#include <memlayout.h>
#include <sync.h>

// pre define
struct mm_struct;

// the virtual continuous memory area(vma), [vm_start, vm_end),
// addr belong to a vma means  vma.vm_start<= addr <vma.vm_end
struct vma_struct {
  struct mm_struct *vm_mm;  // 记录 vma 隶属的页目录项
  uintptr_t vm_start;       // 记录 vma 的起始地址
  uintptr_t vm_end;         // 记录 vma 的结束地址
  uint32_t vm_flags;        // 记录 vma 的标记位
  list_entry_t list_link;   // 记录该进程所有 vma 的链表
};

#define le2vma(le, member) to_struct((le), struct vma_struct, member)

#define VM_READ 0x00000001
#define VM_WRITE 0x00000002
#define VM_EXEC 0x00000004

// the control struct for a set of vma using the same PDT
struct mm_struct {
  list_entry_t mmap_list;         // 记录该进程所有 vma 的链表
  struct vma_struct *mmap_cache;  // 记录当前访问的 vma
  pde_t *pgdir;                   // 记录该进程页目录表的起始地址
  int map_count;                  // 记录该进程的 vma 数目
  void *sm_priv;                  // 循环队列头指针
};

struct vma_struct *find_vma(struct mm_struct *mm, uintptr_t addr);
struct vma_struct *vma_create(uintptr_t vm_start, uintptr_t vm_end,
                              uint32_t vm_flags);
void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma);

struct mm_struct *mm_create(void);
void mm_destroy(struct mm_struct *mm);

void vmm_init(void);

int do_pgfault(struct mm_struct *mm, uint32_t error_code, uintptr_t addr);

extern volatile unsigned int pgfault_num;
extern struct mm_struct *check_mm_struct;
#endif /* !__KERN_MM_VMM_H__ */
