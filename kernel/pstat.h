#ifndef _PSTAT_H_
#define _PSTAT_H_

#include "param.h"

struct pstat {
  int pid[NPROC];
  int inuse[NPROC];
  int priority[NPROC];
  int wtime[NPROC];
  int starving[NPROC];
  char name[NPROC][16];
  int state[NPROC]; // Adding state to visualize RUNNABLE/RUNNING etc.
};

#endif // _PSTAT_H_
