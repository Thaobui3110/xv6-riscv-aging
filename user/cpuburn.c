#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  long i = 0;

  //printf("cpubound: starting, pid = %d\n", getpid());

  // Vòng lặp vô hạn, chỉ tính toán, không sleep, không I/O nhiều
  while(1){
    i++;

    // Thỉnh thoảng in ra cho biết nó vẫn chạy
    if(i % 100000000 == 0){
      printf("cpubound: still running, i=%d\n", (int)i);
    }
  }

  // thực ra program này không bao giờ tới đây
  exit(0);
}

