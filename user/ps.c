#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  ps();   // gọi syscall ps() → kernel chạy procdump()
  exit(0);
}

