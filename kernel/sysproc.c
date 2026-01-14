#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "pstat.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_set_priority(void)
{
  int pid, new_priority;
  int old_priority = -1;
  struct proc *p;
  extern struct proc proc[NPROC];
  
  argint(0, &pid);
  argint(1, &new_priority);

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED){
      old_priority = p->priority;
      p->priority = new_priority;
      
      // Reset ctime to current ticks to ensure fairness/correctness for FCFS 
      // within the new priority level? 
      // Requirement says: "Processes with same priority are executed on first come first served basis."
      // If I move a process to a new priority queue, it is technically "arriving" now.
      // But the implementation plan approved said: "ctime remains unchanged as it reflects creation time."
      // So I will NOT update ctime.
      
      release(&p->lock);
      
      // Preemption Logic:
      // If the process whose priority we changed has higher priority (lower value)
      // than the currently running process, we yield to allow it to run.
      // Or if we lowered our own priority, we yield to check if there's someone better.
      if(p->priority < myproc()->priority || (p == myproc() && new_priority > old_priority)) { //myproc là tiến trình đang chạy
        yield();
      }
      
      return old_priority;
    }
    release(&p->lock);
  }
  
  return -1;
}

uint64
sys_getpinfo(void)
{
  uint64 addr;
  struct pstat pst;
  struct proc *p;
  extern struct proc proc[NPROC];

  argaddr(0, &addr);

  int i = 0;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    pst.pid[i] = p->pid;
    pst.inuse[i] = (p->state != UNUSED);
    pst.priority[i] = p->priority;
    pst.wtime[i] = p->wtime;
    pst.rtime[i] = p->rtime;
    pst.starving[i] = p->starving;
    pst.state[i] = p->state;
    safestrcpy(pst.name[i], p->name, sizeof(p->name));
    release(&p->lock);
    i++;
  }

  if(copyout(myproc()->pagetable, addr, (char *)&pst, sizeof(pst)) < 0)
    return -1;
  
  return 0;
}

uint64
sys_process_aging(void)
{
  int new_val;
  extern int aging_enabled;
  int old_val = aging_enabled;

  argint(0, &new_val);
  
  if(new_val >= 0) {
      aging_enabled = new_val;
  }
  return old_val; 
}
