#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGBLOCKS    (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define USERSTACK    1     // user stack pages

#define AGING_THRESHOLD      10   // wtime > 10 ticks thì bắt đầu tăng priority (reduced for testing)
#define STARVING_THRESHOLD   30   // wtime > 30 ticks coi là starvation (reduced for testing)
#define AGING_STEP       1     // mỗi lần boost giảm priority 1
#define MIN_PRIORITY         0
#define MAX_PRIORITY         100  // tuỳ cách m định nghĩa
#define DEBUG_AGING 0

#define AGING_ENABLE 1        // set to 1 to enable automatic aging in clockintr

#define DEFAULT_PRIORITY 60




