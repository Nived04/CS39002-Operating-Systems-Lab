#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>	
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>

#define KEY_STRING "/"
#define VAL 65

int get_random_number(int l, int r) {
    return (rand() % (r - l + 1)) + l;
}

// default num of followers
int num_foll = 10;
bool sum_hash[1000];

int main(int argc, char* argv[]) {
    if(argc == 2) {
        num_foll = atoi(argv[1]);
    }    

    // initialize sum_hash to 0
    memset(sum_hash, 0, sizeof(sum_hash));

    // create a shared array M with a custom key /
    key_t key;
    key = ftok(KEY_STRING, VAL);
    int shmid = shmget(key, 105 * sizeof(int), 0777|IPC_CREAT|IPC_EXCL);

    // attach the shared memory
    int *M = (int*)shmat(shmid, 0, 0);

    // initialize the shared memory
    M[0] = num_foll;
    M[1] = 0;
    M[2] = 0;

    while(M[1] != num_foll) {
        sleep(1); // wait until all the followers join
    }
    
    printf("Shmid of the code (in case required) : %d\n", shmid);
    while(1) {
        // leader's turn initially
        M[3] = get_random_number(1, 99);
        // next turn: Follower 1
        M[2] = 1;
        
        while(M[2] != 0) sleep(1);

        // find the sum of the values of followers
        int current_sum = 0;
        for(int i=0; i<=num_foll; i++) {
            current_sum += M[3+i];
            if(i!=num_foll) {
                printf("%d + ", M[3+i]);
            }
            else {
                printf("%d = %d\n", M[3+i], current_sum);
            }
        }

        if(sum_hash[current_sum] == 1) {
            break;
        }
        else {
            sum_hash[current_sum] = 1;
        }
    }

    // leader asks all followers to leave
    M[2] = -1;
    while(M[2] != 0) sleep(1);

    // free shared memory segment
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}