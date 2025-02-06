#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>	
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define KEY_STRING "/"
#define VAL 65

// default num of child followers
int child_followers = 1;

int main(int argc, char* argv[]) {
    if(argc == 2) {
        child_followers = atoi(argv[1]);
    }

    // get the shared memory id (not created)
    key_t key;
    key = ftok(KEY_STRING, VAL);
    int shmid = shmget(key, 105 * sizeof(int), 0777);

    // if shmid is -1, leader not yet formed
    if(shmid == -1) {
        printf("Leader not yet formed\n");
        exit(1);
    }

    // attach the shared memory
    int *M = (int*)shmat(shmid, 0, 0);
    int num_followers = M[0];

    for(int i=1; i<=child_followers; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            srand(time(NULL)^getpid());

            if(M[1] == num_followers) {
                printf("follower error: %d followers already joined\n", num_followers);
                shmdt(M);
                exit(1);
            }

            int id = ++M[1];
            printf("follower %d joins\n", id);

            // child doesnt leave until it sees a -id in M[2] 
            while(M[2] != -1*id) {
                if(M[2] == id) {
                    M[3+id] =  rand() % 9 + 1;
                    M[2] = (id + 1) % (num_followers + 1);
                }
            }

            printf("follower %d leaves\n", id);
            // sets to -(id+1) telling next follower to leave
            M[2] = -1*((id+1) % (num_followers + 1));
    
            shmdt(M);
            
            exit(0);
        }

        else if(pid < 0) {
            printf("fork error\n");    
            shmdt(M);
            exit(1);
        }
    }

    for(int i=1; i<=child_followers; i++) {
        wait(NULL);
    }

    return 0;
}