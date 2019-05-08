#include <intr.h>
#include <x86.h>

/* *
 * intr_enable - enable irq interrupt
 * 开启中断
 * */
void intr_enable(void) { sti(); }

/* *
 * intr_disable - disable irq interrupt
 * 关闭中断
 * */
void intr_disable(void) { cli(); }
