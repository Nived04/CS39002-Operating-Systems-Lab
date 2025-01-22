#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

const char* status[4] = {"CATCH", ".... ", "MISS ", "     "};

// initially the child is playing
int myStat = 1; 
// store the total children, next child to send the print signal and the curr child number
int n, passOnPid, curr_child;

/* The signal handler for the child process */
void childSigHandler (int sig) {
    // on receiving sigint, either the child is out of game or the child is the winner 
    if(sig == SIGINT) {
        if(myStat == 1) {
            printf("+++ Child %d: Yay! I am the winner!\n", curr_child);
            fflush(stdout);
        }
        exit(0);
    }

    // on receiving sigusr1, print the status of the child and send sigusr1 to the next child
    if(sig == SIGUSR1) {
        printf("%s   ", status[myStat]);
        fflush(stdout);

        // if it is the nth child, send the sigint interrupt to the dummy child so that parent can end waiting
        if(curr_child == n) {
            FILE* f = fopen("dummycpid.txt", "r");
            fscanf(f, "%d", &passOnPid);
            fclose(f);
        }

        // if stat is "0(caught)", change to "playing(1)", else if stat is "2(missed)", change to "out of game(3)"
        if(myStat%2 == 0) myStat = myStat+1;
        kill (passOnPid, (curr_child == n) ? SIGINT : SIGUSR1);
    }
    // on receiving sigusr2, change the status of the child to out of game with 20% probability
    else if(sig == SIGUSR2) {
        myStat = (rand()%10 < 2) ? 2 : 0;
        kill(getppid(), (myStat == 2) ? SIGUSR2 : SIGUSR1);
    }  
}

int main() {
    sleep(1);

    srand((unsigned int)time(NULL) ^ (getpid()<<16));

    // reading the PIDs of sibling processes so as to find the PID of the next child in the circle
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

    // registering signal handlers for SIGUSR1, SIGUSR2 and SIGINT
    signal(SIGUSR1, childSigHandler);
    signal(SIGUSR2, childSigHandler);
    signal(SIGINT, childSigHandler);

    while(1) {
        pause();
    }
}