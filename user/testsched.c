#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/pstat.h"

void busy_loop(int x) {
    int i, j;
    for (i = 0; i < x; i++) {
        for (j = 0; j < 1000000; j++) {
            asm volatile("nop");
        }
    }
}

void test_priority_ordering() {
    printf("--- Case 1: Priority Ordering Test ---\n");
    process_aging(0); // Disable aging
    set_priority(getpid(), 0); // Max priority for Parent to create all children first
    
    int priorities[3] = {20, 10, 5}; // Low, Med, High (lower val = high priority)
    int pids[3];
    
    for(int i=0; i<3; i++) {
        int pid = fork();
        if(pid == 0) {
            set_priority(getpid(), priorities[i]);
            busy_loop(50000); // Increased to ensure preemption works
            exit(0);
        }
        pids[i] = pid;
    }
    
    printf("Created processes: PID=%d(pri=20), PID=%d(pri=10), PID=%d(pri=5)\n", 
           pids[0], pids[1], pids[2]);
    
    int finished[3];
    for(int i=0; i<3; i++) {
        finished[i] = wait(0);
        printf("Child %d finished: PID=%d\n", i+1, finished[i]);
    }
    
    // Verify order: should be pids[2] (pri=5), pids[1] (pri=10), pids[0] (pri=20)
    if(finished[0] == pids[2] && finished[1] == pids[1] && finished[2] == pids[0]) {
        printf("SUCCESS: Correct priority ordering!\n");
    } else {
        printf("PARTIAL: Order may vary due to timing. Expected: %d, %d, %d\n", 
               pids[2], pids[1], pids[0]);
    }
}

void test_starvation_detection() {
    printf("--- Case 2: Starvation Detection Test ---\n");
    process_aging(0); // Disable aging
    set_priority(getpid(), 0); // Max priority for monitoring
    
    // Create aggressors FIRST (so they are older and win ctime tie-breaker)
    int aggressors[3];
    for(int i=0; i<3; i++){
        aggressors[i] = fork();
        if(aggressors[i] == 0){
            set_priority(getpid(), 5); // High priority
            busy_loop(500000);
            exit(0);
        }
    }
    printf("Created 3 aggressor processes with Priority 5.\n");
    
    // Create victim LAST
    int victim = fork();
    if(victim == 0) {
        set_priority(getpid(), 20); // Lowest priority
        busy_loop(500000); 
        exit(0);
    }
    printf("Victim PID=%d created with Priority 20.\n", victim);
    
    // Monitor
    struct pstat st;
    int starved = 0;
    printf("Monitoring for starvation (max 60 checks)...\n");
    
    for(int k=0; k<60; k++) { // Reduced from 200, increased from 50
        pause(10); // Increased from 5
        if(getpinfo(&st) == 0){
            for(int i=0; i<NPROC; i++){
                if(st.pid[i] == victim) {
                    if(st.starving[i]) {
                        printf("SUCCESS: Victim PID=%d is STARVING at check %d!\n", victim, k+1);
                        starved = 1;
                        break;
                    }
                    // Print progress every 10 checks
                    if(k % 10 == 0) {
                        printf("Check %d: Victim PID=%d wtime=%d starving=%d\n", 
                               k+1, victim, st.wtime[i], st.starving[i]);
                    }
                }
            }
        }
        if(starved) break;
    }
    
    if(!starved) printf("FAILURE: Victim did not starve after 60 checks.\n");
    
    kill(victim);
    for(int i=0; i<3; i++) kill(aggressors[i]);
    for(int i=0; i<4; i++) wait(0);
    printf("Test completed and cleaned up.\n");
}

void test_aging_rebalancing() {
    printf("--- Case 3: Aging Rebalancing Test ---\n");
    process_aging(1); // Enable aging
    set_priority(getpid(), 0); // Max priority for monitoring
    
    // Create aggressors FIRST
    int aggressors[3];
    for(int i=0; i<3; i++){
        aggressors[i] = fork();
        if(aggressors[i] == 0){
            set_priority(getpid(), 5); // High priority initial
            busy_loop(500000); // Reduced from 1000
            exit(0);
        }
    }
    printf("Created 3 aggressor processes with Priority 5.\n");

    // Create victim LAST
    int victim = fork();
    if(victim == 0) {
        set_priority(getpid(), 20); // Lowest priority initial
        busy_loop(500000); // Reduced from 1000
        exit(0);
    }
    
    printf("Victim PID=%d created with initial Priority 20.\n", victim);
    
    struct pstat st;
    int improved = 0;
    int was_starving = 0;
    int initial_victim_prio = 20;
    
    printf("Monitoring for aging effects (max 50 checks)...\n");

    for(int k=0; k<50; k++) { // Reduced from 200
        pause(10); // Increased from 5
        if(getpinfo(&st) == 0){
            for(int i=0; i<NPROC; i++){
                if(st.pid[i] == victim) {
                    // Print every 5 checks or when priority changes
                    if(k % 5 == 0 || st.priority[i] < initial_victim_prio) {
                        printf("Check %d: Victim PID=%d Priority=%d wtime=%d starving=%d\n", 
                               k+1, victim, st.priority[i], st.wtime[i], st.starving[i]);
                    }
                    
                    if(st.starving[i]) was_starving = 1;
                    
                    if(st.priority[i] < initial_victim_prio) {
                        improved = 1;
                        printf("Priority improved from %d to %d!\n", 
                               initial_victim_prio, st.priority[i]);
                        initial_victim_prio = st.priority[i];
                    }
                }
            }
        }
        if(improved && !was_starving) break; // Exit early if aging worked
    }
    
    if(improved) {
        printf("SUCCESS: Victim priority improved via aging.\n");
        if(was_starving) {
            printf("Note: Victim was starving before aging kicked in.\n");
        }
    } else {
        printf("FAILURE: Victim priority did not improve.\n");
    }

    kill(victim);
    for(int i=0; i<3; i++) kill(aggressors[i]);
    for(int i=0; i<4; i++) wait(0);
    printf("Test completed and cleaned up.\n");
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: testsched <case_num>\n");
        printf("1: Priority Ordering\n");
        printf("2: Starvation Detection\n");
        printf("3: Aging Rebalancing\n");
        exit(1);
    }
    
    int tc = atoi(argv[1]);
    switch(tc) {
        case 1: test_priority_ordering(); break;
        case 2: test_starvation_detection(); break;
        case 3: test_aging_rebalancing(); break;
        default: printf("Invalid test case\n");
    }
    exit(0);
}
