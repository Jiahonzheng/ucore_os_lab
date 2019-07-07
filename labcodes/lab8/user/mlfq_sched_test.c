#include <ulib.h>
#include <stdio.h>

int main(void) {
  int i, pid;
  for (i = 0; i < 6; i++) {
    if ((pid = fork()) == 0) {
      cprintf("I am Child %d.\n", i);
      while(1);
      exit(0);
    }
  }
  for (; i > 0; i--) {
    wait();
  }
  wait();
  return 0;
}