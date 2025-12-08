#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  //printf("iobound: starting, pid = %d\n", getpid());

  while(1){
    printf("iobound: doing some I/O, then sleep\n");
    // dùng pause thay cho sleep
    pause(50);
  }

  exit(0);
}

