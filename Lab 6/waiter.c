#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>

#define M_PATH "sem.c"
#define CO_PATH "cook.c"
#define W_PATH "waiter.c"
#define CU_PATH "customer.c"
#define SEM_KEY_VAL 66

#define KEY_STRING "/"
#define KEY_VAL 65

#define U 0
#define V 1
#define W 2
#define X 3
#define Y 4

int shmid;
struct sembuf pop, vop;
int mutex_sem, cook_sem, waiter_sem, customer_sem;

void print_time(int min) {
    int start = 11;
    int hours = min/60;
    int mins = min%60;
    if(hours < 1) {
        printf("[%d:%02d am] ", start+hours, mins);
    }
    else if(hours == 1) {
        printf("[%d:%02d pm] ", start+hours, mins);
    }
    else {
        printf("[%02d:%02d pm] ", hours-1, mins);
    }
    fflush(stdout);
}

void get_semaphores() {
    mutex_sem    = semget(ftok(M_PATH, SEM_KEY_VAL), 1, 0);
    cook_sem     = semget(ftok(CO_PATH, SEM_KEY_VAL), 1, 0);
    waiter_sem   = semget(ftok(W_PATH, SEM_KEY_VAL), 5, 0);
    customer_sem = semget(ftok(CU_PATH, SEM_KEY_VAL), 200, 0);
}

void get_shmid() {
    key_t key = ftok(KEY_STRING, KEY_VAL);
    shmid = shmget(key, 0, 0);
}

void wmain(int i) {
    int *M = (int *)shmat(shmid, 0, 0);

    print_time(0);
    printf("Waiter %c is ready\n", i + 'U');

    while(1) {
        struct sembuf wop = {i, -1, 0};
        semop(waiter_sem, &wop, 1);
        
        int fr = 4*(i+1);
        int po = 4*(i+1) + 1;
        int f = 4*(i+1) + 2;
        int b = 4*(i+1) + 3;
        
        semop(mutex_sem, &pop, 1);
        
        if(M[fr] != 0) {
            
            print_time(M[0]);
            printf("Waiter %c: Serving food to Customer %d\n", i+'U', M[fr]);
            
            struct sembuf cop = {M[fr]-1, 1, 0};
            semop(customer_sem, &cop, 1);
            
            M[fr] = 0;
            M[27+i]--;
            if((M[0] > 240) && (M[f] == M[b]) && (M[27+i] == 0)) {
                print_time(M[0]);
                printf("Waiter %c leaving (no more customers to serve)\n", i+'U');
                semop(mutex_sem, &vop, 1);
                break;
            }
        }
        else if(M[po] != 0) {
            int customer_id = M[M[f]];
            int customer_count = M[M[f]+1];
            M[f] = M[f] + 2;
            M[po]--;    
            int current_time = M[0];
            semop(mutex_sem, &vop, 1);

            usleep(100000);
            current_time += 1;
            struct sembuf cop = {customer_id-1, 1, 0};
            semop(customer_sem, &cop, 1);

            print_time(current_time);
            printf("Waiter %c: Placing order for Customer %d (Count %d)\n", i+'U', customer_id, customer_count);

            semop(mutex_sem, &pop, 1);
            if(M[0] <= current_time) {
                M[0] = current_time;
            }
            else {
                printf("*** INVALID TIME");
            }
            
            M[M[25]] = i;
            M[M[25]+1] = customer_id;
            M[M[25]+2] = customer_count;
            M[25] += 3; // enqueue
            M[27+i]++;
            semop(cook_sem, &vop, 1);
        }
        else {
            print_time(M[0]);
            printf("Waiter %c leaving (no more customers to serve)\n", i+'U');
            semop(mutex_sem, &vop, 1);
            break;
        }
        semop(mutex_sem, &vop, 1);
    }

    shmdt(M);
    exit(0);
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0); // in case output is redirected to a file, then \n will flush

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    get_semaphores();
    get_shmid();

    for(int i=0; i<5; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            wmain(i);
        }
    }

    for(int i=0; i<5; i++) {
        wait(NULL);
    }
}