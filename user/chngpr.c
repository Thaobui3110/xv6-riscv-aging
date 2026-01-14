#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc < 3){
    fprintf(2, "Usage: chngpr <pid> <priority>\n");
    exit(1);
  }

  int pid = atoi(argv[1]);
  int priority = atoi(argv[2]);

  int old_priority = set_priority(pid, priority);

  if(old_priority == -1) {
      fprintf(2, "chngpr: failed to set priority for pid %d\n", pid);
      exit(1);
  }

  printf("Old priority: %d\n", old_priority);

  exit(0);
}
