#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv){
  int lowpid;

  // spawn one long-lived low-priority worker
  lowpid = fork();
  if(lowpid == 0){
    // child: low priority
    setpriority(getpid(), 90);
    int iter = 0;
    while(1){
      if(iter % 100 == 0)
        printf("low-worker running iter=%d\n", iter);
      // small busy work
      for(volatile int i=0;i<200000;i++);
      // sleep 1 tick so we don't monopolize if we ever get CPU
      pause(1);
      iter++;
    }
    exit(0);
  }

  // parent: create a stream of short high-priority jobs to try to starve the low-priority
  for(int j = 0; j < 300; j++){
    int pid = fork();
    if(pid == 0){
      // high-priority short job
      setpriority(getpid(), 5);
      // short CPU-bound burst
      for(volatile int k = 0; k < 2000000; k++);
      // exit quickly
      exit(0);
    } else if(pid > 0){
      // small pause to keep the flood continuous but not instantaneous
      pause(0);
    } else {
      // fork failed
      printf("fork failed\n");
    }
  }

  // parent waits a while then print ps periodically
  for(int t=0;t<30;t++){
    pause(2);
    printf("--- PS at t=%d ---\n", t);
    ps();
  }

  // kill low worker and wait for children
  kill(lowpid);
  for(int i=0;i<300;i++)
    wait(0);

  return 0;
}
