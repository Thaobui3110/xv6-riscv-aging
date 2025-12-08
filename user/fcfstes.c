#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("FCFS starvation test started\n");

  // Tạo 1 process long-running CPU-bound
  int pid = fork();
  if(pid == 0){
    printf("Long process started (PID=%d)\n", getpid());
    // nhiệm vụ: giữ CPU cực lâu
    for(long long i = 0; i < 1000000000000LL; i++){
      // do nothing, chỉ đốt CPU
    }
    printf("Long process done\n");
    exit(0);
  }

  // Cha sleep để đảm bảo long process chạy trước
  pause(10);

  // Tạo nhiều short processes
  for(int i = 0; i < 5; i++){
    int p = fork();
    if(p == 0){
      printf("Short process %d started (PID=%d)\n", i, getpid());
      pause(20);
      printf("Short process %d finished\n", i);
      exit(0);
    }
  }

  // Cha: quan sát bằng ps
  for(int i = 0; i < 50; i++){
    pause(10);
    printf("==== ps at iteration %d ====\n", i);
    ps();   // giả sử m đã có syscall ps() và prototype trong user.h
  }

  // Đợi tất cả process con
  for(int i = 0; i < 6; i++){
    wait(0);
  }

  exit(0);
}

