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
}

void initialize_semaphores() {
    mutex_sem    = semget(ftok(M_PATH, SEM_KEY_VAL), 1, 0777|IPC_CREAT);
    cook_sem     = semget(ftok(CO_PATH, SEM_KEY_VAL), 1, 0777|IPC_CREAT);
    waiter_sem   = semget(ftok(W_PATH, SEM_KEY_VAL), 5, 0777|IPC_CREAT);
    customer_sem = semget(ftok(CU_PATH, SEM_KEY_VAL), 200, 0777|IPC_CREAT);

    if(mutex_sem == -1 || cook_sem == -1 || waiter_sem == -1 || customer_sem == -1) {
        perror("Error in creating semaphore");
        exit(1);
    }

    semctl(mutex_sem, 0, SETVAL, 1);            // initially mutex value is 1, signifying that it is unlocked/awake
    semctl(cook_sem, 0, SETVAL, 0);             // initially cook is sleeping
    for(int i=0; i<5; i++) {
        semctl(waiter_sem, i, SETVAL, 0);       // initially all waiters are sleeping
    }
    for(int i=0; i<200; i++) {
        semctl(customer_sem, i, SETVAL, 0);     // initially all customers are sleeping
    }
}

void initialize_shared_memory() {
    key_t key;
    key = ftok(KEY_STRING, KEY_VAL);
    shmid = shmget(key, 2000*sizeof(int), 0777|IPC_CREAT);

    int* M = (int*)shmat(shmid, 0, 0);

    M[0] = 0;   // initial time 
    M[1] = 10;  // number of empty tables
    M[2] = 0;   // waiter number to serve the next customer
    M[3] = 0;   // number of orders pending for the cooks

    for(int i=4; i<24; i+=4) {
        M[i] = 0;   // FR
        M[i+1] = 0; // PO
        M[i+2] = 100*(2*(i/4) - 1); // F
        M[i+3] = 100*(2*(i/4) - 1); // B
    } 

    M[24] = 1100;   // F for cooking queue
    M[25] = 1100;   // B for cooking queue

    M[26] = 2;      // number of cooks

    // unprepared orders for waiters U, V, W, X, Y
    M[27] = 0;   
    M[28] = 0;
    M[29] = 0;
    M[30] = 0;
    M[31] = 0;

    for(int i=100; i<2000; i++) M[i] = 0;

    shmdt(M);
}   

void cmain(int i) {
    int *M = (int*)shmat(shmid, 0, 0);

    char cook = i+'C';

    print_time(0);
    printf("Cook %c is ready\n", cook);
    while(1) {
        int waiter_num, customer_id, customer_count;
        semop(cook_sem, &pop, 1);
        
        semop(mutex_sem, &pop, 1);
        if((M[0] > 240) && (M[24] == M[25])) {
            print_time(M[0]);
            printf("Cook %c leaving\n", cook);
            M[26]--;
            if(M[26] == 0) {
                for(int i=0; i<5; i++) {
                    struct sembuf wop = {i, 1, 0};
                    semop(waiter_sem, &wop, 1);
                }
            }
            semop(mutex_sem, &vop, 1);
            break;
        }
        semop(mutex_sem, &vop, 1);
    
        semop(mutex_sem, &pop, 1);
        waiter_num = M[M[24]];
        customer_id = M[M[24]+1];
        customer_count = M[M[24]+2];   
        M[24] += 3; // incrementing the front of the queue (dequeuing)
        int current_time = M[0];
        semop(mutex_sem, &vop, 1);

        // Prepare food for the order
        print_time(current_time);
        printf("Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n", cook, waiter_num+'U', customer_id, customer_count);
        int cook_time = 5*customer_count;
        usleep(cook_time*100000);
        current_time += cook_time;
        print_time(current_time);
        printf("Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n", cook, waiter_num+'U', customer_id, customer_count);

        // Notify the waiter in the order that food is ready
        semop(mutex_sem, &pop, 1);
        M[4*(waiter_num+1)] = customer_id;
        if(M[0] <= current_time) {
            M[0] = current_time;
        }
        else {
            printf("*** INVALID TIME");
        }
        struct sembuf waiter_op = {waiter_num, 1, 0};
        semop(waiter_sem, &waiter_op, 1);
        semop(mutex_sem, &vop, 1);

        semop(mutex_sem, &pop, 1);
        if((M[0] > 240) && (M[24] == M[25])) {
            print_time(M[0]);
            printf("Cook %c leaving\n", cook);
            M[26]--;
            if(M[26] == 0) {
                for(int i=0; i<5; i++) {
                    struct sembuf wop = {i, 1, 0};
                    semop(waiter_sem, &wop, 1);
                }
            }
            else {
                for(int i=0; i<2; i++) {
                    struct sembuf cop = {i, 1, 0};
                    semop(cook_sem, &cop, 1);
                }
            }
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

    initialize_semaphores();
    initialize_shared_memory();

    for(int i=0; i<2; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            cmain(i);
        }
    }

    for(int i=0; i<2; i++) {
        wait(NULL);
    }
}