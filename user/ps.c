#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct pstat st;
  static char *states[] = {
    "UNUSED", "USED", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"
  };

  if(getpinfo(&st) < 0){
    fprintf(2, "ps: getpinfo failed\n");
    exit(1);
  }

  printf("PID\tName\tPriority\tState\t\tWait Time\tStarving\n");
  for(int i = 0; i < NPROC; i++){
    if(st.inuse[i]){
      printf("%d\t%s\t%d\t\t%s", st.pid[i], st.name[i], st.priority[i], states[st.state[i]]);
      
      // Align tabs based on state name length if needed, simplistic approach:
      if(strlen(states[st.state[i]]) < 8) printf("\t");
      
      printf("\t%d\t\t%s\n", st.wtime[i], st.starving[i] ? "Yes" : "No");
    }
  }
  exit(0);
}
