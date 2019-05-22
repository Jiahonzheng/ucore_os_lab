#ifndef __KERN_MM_MEMLAYOUT_H__
#define __KERN_MM_MEMLAYOUT_H__

/* This file contains the definitions for memory management in our OS. */

/* global segment number */
#define SEG_KTEXT 1  //内核代码段
#define SEG_KDATA 2  // 内核数据段
#define SEG_UTEXT 3  // 用户代码段
#define SEG_UDATA 4  // 用户数据段
#define SEG_TSS 5    // 任务选择段

/* *
 * global descrptor numbers
 * 全局描述符编号
 * */
#define GD_KTEXT ((SEG_KTEXT) << 3)  // kernel text
#define GD_KDATA ((SEG_KDATA) << 3)  // kernel data
#define GD_UTEXT ((SEG_UTEXT) << 3)  // user text
#define GD_UDATA ((SEG_UDATA) << 3)  // user data
#define GD_TSS ((SEG_TSS) << 3)      // task segment selector

/* *
 * 权限级别
 * */
#define DPL_KERNEL (0)
#define DPL_USER (3)

/* *
 * 代码段描述符
 * */
#define KERNEL_CS ((GD_KTEXT) | DPL_KERNEL)
#define KERNEL_DS ((GD_KDATA) | DPL_KERNEL)
#define USER_CS ((GD_UTEXT) | DPL_USER)
#define USER_DS ((GD_UDATA) | DPL_USER)

/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G ------------------> +---------------------------------+
 *                            |                                 |
 *                            |         Empty Memory (*)        |
 *                            |                                 |
 *                            +---------------------------------+ 0xFB000000
 *                            |   Cur. Page Table (Kern, RW)    | RW/-- PTSIZE
 *     VPT -----------------> +---------------------------------+ 0xFAC00000
 *                            |        Invalid Memory (*)       | --/--
 *     KERNTOP -------------> +---------------------------------+ 0xF8000000
 *                            |                                 |
 *                            |    Remapped Physical Memory     | RW/-- KMEMSIZE
 *                            |                                 |
 *     KERNBASE ------------> +---------------------------------+ 0xC0000000
 *                            |                                 |
 *                            |                                 |
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.
 *
 * */

/* All physical memory mapped at this address */
#define KERNBASE 0xC0000000
#define KMEMSIZE 0x38000000  // the maximum amount of physical memory
#define KERNTOP (KERNBASE + KMEMSIZE)

/* *
 * Virtual page table. Entry PDX[VPT] in the PD (Page Directory) contains
 * a pointer to the page directory itself, thereby turning the PD into a page
 * table, which maps all the PTEs (Page Table Entry) containing the page
 * mappings for the entire virtual address space into that 4 Meg region starting
 * at VPT.
 * */
#define VPT 0xFAC00000

#define KSTACKPAGE 2                      // # of pages in kernel stack
#define KSTACKSIZE (KSTACKPAGE * PGSIZE)  // sizeof kernel stack

#ifndef __ASSEMBLER__

#include <atomic.h>
#include <defs.h>
#include <list.h>

/* *
 * 页表项类型
 * 高 20 位为页面编号，低 12 位为页表项标记
 * */
typedef uintptr_t pte_t;

/* *
 * 页表目录类型
 * 高 20 位为页表索引，低 12 位为页表项标记
 * */
typedef uintptr_t pde_t;
typedef pte_t swap_entry_t;  // the pte can also be a swap entry

/* *
 * BIOS 15H 中断的一些相关常量
 * see bootasm.S
 * */
#define E820MAX 20  // number of entries in E820MAP
#define E820_ARM 1  // address range memory
#define E820_ARR 2  // address range reserved

struct e820map {
  int nr_map;
  struct {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
  } __attribute__((packed)) map[E820MAX];
};

/* *
 * struct Page - Page descriptor structures. Each Page describes one
 * physical page. In kern/mm/pmm.h, you can find lots of useful functions
 * that convert Page to other data types, such as phyical address.
 *
 * 页描述符
 * */
struct Page {
  int ref;  // 页帧的引用计数器，若被页表引用的次数为 0，那么这个页帧将被释放
  uint32_t flags;              // 标志位
  unsigned int property;       // 连续内存空闲块大小
  list_entry_t page_link;      // free list link
  list_entry_t pra_page_link;  // 空闲块列表 free_list 的链表项
  uintptr_t pra_vaddr;         // used for pra (page replace algorithm)
};

/* Flags describing the status of a page frame */
#define PG_reserved \
  0  // if this bit=1: the Page is reserved for kernel, cannot be used in
     // alloc/free_pages; otherwise, this bit=0
#define PG_property \
  1  // if this bit=1: the Page is the head page of a free memory block(contains
     // some continuous_addrress pages), and can be used in alloc_pages; if this
     // bit=0: if the Page is the the head page of a free memory block, then
     // this Page and the memory block is alloced. Or this Page isn't the head
     // page.

// 将页面设置为保留页面。供给内存分配器管理
#define SetPageReserved(page) set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page) clear_bit(PG_reserved, &((page)->flags))

// 判断页面是否保留。保留页面不可用于分配，保留的页面给 pmm_manager 进行处理
#define PageReserved(page) test_bit(PG_reserved, &((page)->flags))

// 将该页面标记为空闲内存块的头页面
#define SetPageProperty(page) set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page) clear_bit(PG_property, &((page)->flags))
#define PageProperty(page) test_bit(PG_property, &((page)->flags))

// convert list entry to page
#define le2page(le, member) to_struct((le), struct Page, member)

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
  list_entry_t free_list;  // 空闲块双向链表的头
  unsigned int nr_free;    // 空闲块总数，以页为单位
} free_area_t;

#endif /* !__ASSEMBLER__ */

#endif /* !__KERN_MM_MEMLAYOUT_H__ */
