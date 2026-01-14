#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "Usage: aging <on|off>\n");
    exit(1);
  }

  int enable = -1;
  if(strcmp(argv[1], "on") == 0) {
      enable = 1;
  } else if(strcmp(argv[1], "off") == 0) {
      enable = 0;
  } else {
      fprintf(2, "Usage: aging <on|off>\n");
      exit(1);
  }

  process_aging(enable);
  exit(0);
}
