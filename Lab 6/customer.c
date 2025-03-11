#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
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

int shmid, total_customer_count = 0;
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

void cmain(int customer_id, int arrival_time, int customer_cnt) {
    int* M = (int*)shmat(shmid, 0, 0);

    while(1) {
        semop(mutex_sem, &pop, 1);

        if(M[0] > 240) {
            print_time(M[0]);
            printf("Customer %d leaves (late arrival)\n", customer_id);
            semop(mutex_sem, &vop, 1);
            exit(0);
        }
        else if(M[1] == 0) {
            print_time(M[0]);
            printf("Customer %d leaves (no empty tables)\n", customer_id);
            semop(mutex_sem, &vop, 1);
            exit(0);
        }

        M[1]--;
        int my_waiter = M[2];

        int po = 4*(my_waiter + 1) + 1;
        int b = 4*(my_waiter + 1) + 3;

        M[2] = (M[2] + 1)%5;

        M[po]++; // PO increase by 1
        M[M[b]] = customer_id;
        M[M[b] + 1] = customer_cnt;
        M[b] += 2; // enqueue
        int arrival_time = M[0]; 
        semop(mutex_sem, &vop, 1);

        struct sembuf wop = {my_waiter, 1, 0};
        semop(waiter_sem, &wop, 1);

        struct sembuf cop = {customer_id-1, -1, 0};
        semop(customer_sem, &cop, 1);

        // customer places order
        semop(mutex_sem, &pop, 1);
        print_time(M[0]);
        printf("Customer %d: Order placed to Waiter %c\n", customer_id, my_waiter+'U');
        semop(mutex_sem, &vop, 1);
        
        // customer waits for food
        semop(customer_sem, &cop, 1);
        // printf("\nSOMETHING\n");
        semop(mutex_sem, &pop, 1);
        print_time(M[0]);
        printf("Customer %d gets food [Waiting Time = %d]\n", customer_id, M[0] - arrival_time);
        int curr_time = M[0];
        semop(mutex_sem, &vop, 1);

        // eat food for 30 seconds
        usleep(30*100000);
        curr_time += 30;
        print_time(curr_time);
        printf("Customer %d finishes eating and leaves\n", customer_id);
        
        semop(mutex_sem, &pop, 1);
        if(M[0] <= curr_time) {
            M[0] = curr_time;
        }
        else {
            printf("*** INVALID TIME");
        }
        M[1]++;
        semop(mutex_sem, &vop, 1);

        exit(0);
    }
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0); // in case output is redirected to a file, then \n will flush

    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; vop.sem_op = 1;

    get_semaphores();
    get_shmid();

    char *customer_file = "customers.txt";
    FILE* fp = fopen(customer_file, "r");

    int time = 0;

    int* M = (int*)shmat(shmid, 0, 0);

    char line[50];

    while(fgets(line, sizeof(line), fp) != NULL) {
        if(atoi(line) == -1) {
            break;
        }

        int arrival_time, customer_id, customer_cnt;
        sscanf(line, "%d %d %d", &customer_id, &arrival_time, &customer_cnt);
        total_customer_count++;

        usleep((arrival_time - time)*100000);
        time = arrival_time;

        print_time(time);
        printf("Customer %d arrives (count = %d)\n", customer_id, customer_cnt);

        semop(mutex_sem, &pop, 1);
        if(M[0] <= time) {
            M[0] = time;
        }
        else {
            printf("*** INVALID TIME");
        }
        semop(mutex_sem, &vop, 1);

        pid_t pid = fork();
        if(!pid) {
            cmain(customer_id, arrival_time, customer_cnt);
        }
    }

    for(int i=0; i<total_customer_count; i++) {
        wait(NULL);
    }

    shmctl(shmid, IPC_RMID, 0);
    semctl(mutex_sem, 0, IPC_RMID);
    semctl(cook_sem, 0, IPC_RMID);
    semctl(waiter_sem, 0, IPC_RMID);
    semctl(customer_sem, 0, IPC_RMID);
}