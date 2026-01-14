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
    
    int priorities[3] = {20, 10, 5}; // Low, Med, High (lower val = high purity)
    
    for(int i=0; i<3; i++) {
        int pid = fork();
        if(pid == 0) {
            set_priority(getpid(), priorities[i]);
            busy_loop(50); // Simulating CPU work
            exit(0);
        }
    }
    
    for(int i=0; i<3; i++) {
        int wpid = wait(0);
        printf("Child finished: PID=%d\n", wpid);
    }
    printf("Expectation: Children should finish in reverse order of priority values (High priority first).\n");
}

void test_starvation_detection() {
    printf("--- Case 2: Starvation Detection Test ---\n");
    process_aging(0); // Disable aging
    set_priority(getpid(), 0); // Max priority for monitoring
    
    int victim = fork();
    if(victim == 0) {
        set_priority(getpid(), 20); // Lowest priority
        busy_loop(1000); 
        exit(0);
    }
    
    printf("Victim PID=%d created with Priority 20.\n", victim);
    
    // Create aggressors
    int aggressors[3];
    for(int i=0; i<3; i++){
        aggressors[i] = fork();
        if(aggressors[i] == 0){
            set_priority(getpid(), 5); // High priority
            busy_loop(1000);
            exit(0);
        }
    }
    
    // Monitor
    struct pstat st;
    int starved = 0;
    for(int k=0; k<200; k++) {
        pause(5);
        if(getpinfo(&st) == 0){
            for(int i=0; i<NPROC; i++){
                if(st.pid[i] == victim && st.starving[i]) {
                    printf("SUCCESS: Victim PID=%d is STARVING!\n", victim);
                    starved = 1;
                    break;
                }
            }
        }
        if(starved) break;
    }
    
    if(!starved) printf("FAILURE: Victim did not starve.\n");
    
    kill(victim);
    for(int i=0; i<3; i++) kill(aggressors[i]);
    for(int i=0; i<4; i++) wait(0);
}

void test_aging_rebalancing() {
    printf("--- Case 3: Aging Rebalancing Test ---\n");
    process_aging(1); // Enable aging
    set_priority(getpid(), 0); // Max priority for monitoring
    
    int victim = fork();
    if(victim == 0) {
        set_priority(getpid(), 20); // Lowest priority initial
        busy_loop(1000); 
        exit(0);
    }
    
    // Create aggressors
    int aggressors[3];
    for(int i=0; i<3; i++){
        aggressors[i] = fork();
        if(aggressors[i] == 0){
            set_priority(getpid(), 5); // High priority initial
            busy_loop(1000);
            exit(0);
        }
    }
    
    struct pstat st;
    int improved = 0;
    int initial_victim_prio = 20;

    for(int k=0; k<200; k++) {
        pause(5);
        if(getpinfo(&st) == 0){
             for(int i=0; i<NPROC; i++){
                if(st.pid[i] == victim) {
                    printf("Victim PID=%d Priority=%d\n", victim, st.priority[i]);
                    if(st.priority[i] < initial_victim_prio) improved = 1;
                }
            }
        }
    }
    
    if(improved) printf("SUCCESS: Victim priority improved via aging.\n");
    else printf("FAILURE: Victim priority did not improve.\n");

    kill(victim);
    for(int i=0; i<3; i++) kill(aggressors[i]);
    for(int i=0; i<4; i++) wait(0);
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
