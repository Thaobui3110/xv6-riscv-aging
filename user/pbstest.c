// Simple PBS test program (renamed from pbs_test.c)
// Forks several CPU-bound children with different priorities.
// Parent periodically prints process table (ps) to observe priorities and starvation.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void child_work(int id){
  // child 0: CPU-bound (no sleep) to create starvation pressure
  if(id == 0){
    volatile unsigned long x = 0;
    while(1){
      // do some arithmetic to keep the CPU busy
      x += 1;
      // print much more infrequently to avoid console spam
      if ((x & 0x3fffff) == 0) // occasionally print very infrequently
        printf("child %d busy tick\n", id);
    }
  }

  // other children: attempt to remain RUNNABLE so we can observe starvation.
  // We do a small busy loop then call yield() to return to RUNNABLE state
  // quickly (avoid long sleeps which put the process in SLEEPING).
  int tick = 0;
  while(1){
    if(tick % 50 == 0)
      printf("child %d tick %d\n", id, tick);
    // small busy work to avoid tight spinning
    for(volatile int i = 0; i < 20000; i++) ;
    tick++;
    // pause(1) sleeps for 1 tick, then the process becomes RUNNABLE again.
    // Using a short sleep keeps the process frequently RUNNABLE without
    // busy-spinning forever (and avoids using the kernel-only yield()).
    pause(1);
  }
}

int main(int argc, char **argv){
  int n = 3;
  int pids[10];

  for(int i=0;i<n;i++){
    int pid = fork();
    if(pid == 0){
      // child
      child_work(i);
    } else {
      pids[i] = pid;
      // set different priorities: lower number => higher priority
      if(i==0) setpriority(pid, 10); // high priority
      else if(i==1) setpriority(pid, 60); // medium
      else setpriority(pid, 90); // low priority (may starve)
    }
  }

  // Parent monitors the process table periodically
  for(int t=0;t<50;t++){
    pause(10);
    printf("--- PS at t=%d ---\n", t);
    ps();
  }

  // After monitoring, kill children and wait for them to exit
  for(int i=0;i<n;i++){
    kill(pids[i]);
  }
  for(int i=0;i<n;i++)
    wait(0);

  return 0;
}
