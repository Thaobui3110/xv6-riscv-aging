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

void print_ps_table() {
    static struct pstat *st = 0;
    if(st == 0) {
        st = malloc(sizeof(struct pstat));
        if(st == 0) {
            printf("testsched: malloc failed\n");
            return;
        }
    }
    
    static char *states[] = {
        "UNUSED", "USED", "SLEEPING", "RUNNABLE", "RUNNING", "ZOMBIE"
    };

    if(getpinfo(st) < 0){
        printf("testsched: getpinfo failed\n");
        return;
    }

    printf("\nPID\tName\tPriority\tState\t\tWait Time\tRun Time\tStarving\n");
    for(int i = 0; i < NPROC; i++){
        if(st->inuse[i]){
            printf("%d\t%s\t%d\t\t%s", st->pid[i], st->name[i], st->priority[i], states[st->state[i]]);
            if(strlen(states[st->state[i]]) < 8) printf("\t");
            printf("\t%d\t\t%d\t\t%s\n", st->wtime[i], st->rtime[i], st->starving[i] ? "Yes" : "No");
        }
    }
    printf("---------------------------------------------------------------------------\n");
}

void test_priority_ordering() {
    printf("--- Case 1: Priority Ordering Test ---\n");
    process_aging(0); // Disable aging
    set_priority(getpid(), 0); // Parent Highest Prio to control setup
    
    int priorities[3] = {20, 10, 5}; // Low, Med, High (lower val = high purity)


    for(int i=0; i<3; i++) {
        int pid = fork();
        if(pid == 0) {
            // Child just runs
            busy_loop(2000); 
            exit(0);
        } else {
            set_priority(pid, priorities[i]); // Parent sets priority
        }
    }
    
    // Visualization loop
    for(int k=0; k<15; k++) {
         print_ps_table();
         pause(5); 
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
    
    // Fork VICTIM (Low Priority)
    int victim = fork();
    if(victim == 0) {
        while(1) busy_loop(100); 
        exit(0);
    }
    set_priority(victim, 20); // Parent sets priority
    
    printf("Victim PID=%d created with Priority 20.\n", victim);
    
    // Create AGGRESSORS (High Priority)
    int aggressors[3];
    for(int i=0; i<3; i++){
        aggressors[i] = fork();
        if(aggressors[i] == 0){
            while(1) busy_loop(100);
            exit(0);
        }
        set_priority(aggressors[i], 5); // Parent sets priority
    }
    
    // Monitor
    static struct pstat *st = 0;
    if(st == 0) st = malloc(sizeof(struct pstat));
    int starved = 0;
    for(int k=0; k<200; k++) { // Loop for visualization
        // Visualization
        print_ps_table();
        
        pause(5);

        if(getpinfo(st) == 0){
            for(int i=0; i<NPROC; i++){
                if(st->pid[i] == victim && st->starving[i]) {
                    printf("SUCCESS: Victim PID=%d is STARVING!\n", victim);
                    starved = 1;
                    k = 200; // Force loop exit
                    break;
                }
            }
        }
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
        while(1) busy_loop(100); 
        exit(0);
    }
    set_priority(victim, 20);
    
    // Create aggressors
    int aggressors[3];
    for(int i=0; i<3; i++){
        aggressors[i] = fork();
        if(aggressors[i] == 0){
            while(1) busy_loop(100);
            exit(0);
        }
        set_priority(aggressors[i], 5);
    }
    
    static struct pstat *st = 0;
    if(st == 0) st = malloc(sizeof(struct pstat));
    int improved = 0;
    int initial_victim_prio = 20;

    for(int k=0; k<200; k++) {
        // Visualization
        print_ps_table();
        pause(5);

        if(getpinfo(st) == 0){
             for(int i=0; i<NPROC; i++){
                if(st->pid[i] == victim) {
                    if(st->priority[i] < initial_victim_prio) {
                        improved = 1;
                        printf("VICTIM PRIORITY IMPROVED: %d -> %d\n", initial_victim_prio, st->priority[i]);
                    }
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
        printf("Usage: test <case_num>\n");
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
