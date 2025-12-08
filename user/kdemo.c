// user/kdemo.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void) {
  printf("user: call kalloc1()...\n");
  kalloc1();

  printf("user: call psx()...\n");
  psx();

  exit(0);
}
