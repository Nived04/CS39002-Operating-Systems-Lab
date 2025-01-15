#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

const char* status[4] = {"CATCH", ".... ", "MISS ", "     "};

// initially the child is playing
int myStat = 1; 
int passOnPid, n, curr_child;

/* The signal handler for the child process */
void childSigHandler (int sig) {
    if(sig == SIGINT) {
        if(myStat == 1) {
            printf("+++ Child %d: Yay! I am the winner!\n", curr_child);
            fflush(stdout);
        }
        exit(0);
    }

    if(sig == SIGUSR1) {
        printf("%s   ", status[myStat]);
        fflush(stdout);

        if(curr_child == n) {
            FILE* f = fopen("dummycpid.txt", "r");
            fscanf(f, "%d", &passOnPid);
            fclose(f);
        }

        if(myStat%2 == 0) myStat = myStat+1;
        kill (passOnPid, (curr_child == n) ? SIGINT : SIGUSR1);
    }
    else if(sig == SIGUSR2) {
        myStat = (rand()%10 < 2) ? 2 : 0;
        kill(getppid(), (myStat == 2) ? SIGUSR2 : SIGUSR1);
    }  
}

int main() {
    sleep(1);

    srand((unsigned int)time(NULL) ^ (getpid()<<16));

    FILE* f = fopen("childpid.txt", "r");
    fscanf(f, "%d", &n);
    fgetc(f);

    for(int i=0; i<n; i++) {
        char temp[50];
        fgets(temp, 50*sizeof(char), f);
        if(getpid() == atoi(temp)) {
            curr_child = i+1;
            if(i!=n-1) {
                fgets(temp, 50*sizeof(char), f);
                passOnPid = atoi(temp);
                break;
            }
        }
    }

    fclose(f);

    signal(SIGUSR1, childSigHandler);
    signal(SIGUSR2, childSigHandler);
    signal(SIGINT, childSigHandler);

    while(1) {
        pause();
    }
}