#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>

// global variables to store next child to throw the ball and number of children
int throw_to, n;
// array to store PIDs of all children
int *childPIDs;
// array to store "playing or not" status of all children
bool *child_status;
// received status boolen to check if the parent has already received the signal from child 
// so that it can avoid a "hanging" scenario, where the parent pauses but the child has already sent the signal
bool received_signal = 0;

// returns 1 if there are more than 1 children playing, 0 otherwise
bool inPlay() {
    int cnt = 0, i;
    for(i=1; i<=n; i++) {
        if(child_status[i] == 1) cnt++;
    }
    return (cnt == 1) ? 0 : 1;
}

// finds and returns the next child to pass the ball to (in circular order)
int getNextPlayingChild(int curr) {
    int i=0; 
    while(i<n) {
        curr = (curr+1)%n;
        if(curr == 0) curr = n;
        if(child_status[curr] == 1) return curr;
        i++;
    }
    return -1; // situation never arises
}

// function to print some additional formatting
void formattedOutput(int bef) {
    char s1[11 + 8*n], s2[11 + 8*n], temp[10];
 
    memset(s1, ' ', sizeof(s1));  
    s1[sizeof(s1) - 1] = '\0';
    for(int i = 1; i <= n; i++) {
        int pos = (i * 8) - 1;
        sprintf(temp, "%d", i);
        int len = strlen(temp);
        memcpy(&s1[pos - len + 1], temp, len);  
    }   

    s2[0] = '+';
    s2[9+8*n] = '+';
    s2[10+8*n] = '\0';
    for(int i=1; i<=8+8*n; i++) {
        s2[i] = '-';
    }

    if(bef == 1) {
        printf("%s\n", s1);
    }
    else {
        printf("%s\n", s2);
    }
    fflush(stdout);
    return;
}

// custom signal handler for SIGUSR2: which updates the status of the child to out of the game
void signalHandler(int sig) {
    if(sig == SIGUSR1 || sig == SIGUSR2) received_signal = 1;
    if(sig == SIGUSR2) child_status[throw_to] = 0;
    return;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("Please enter a single argument denoting number of children\n");
        exit(1);
    }
    // total number of children
    n = atoi(argv[1]);

    // registering signal handlers for SIGUSR1 and SIGUSR2
    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);

    childPIDs = (int *)malloc((n+1)*sizeof(int));

    FILE *f = fopen("childpid.txt", "w");

    fprintf(f, "%d\n", n);
    fflush(f);

    // fork n children and store their PIDs in childPIDs array and in childpid.txt
    for(int i=0; i<n; i++) {
        int cpid = fork();

        if(cpid) {  // Parent Process
            childPIDs[i+1] = cpid;
            fprintf(f, "%d\n", cpid);
            fflush(f);
        }
        else {      // Child Process
            execlp("./child", "./child", NULL);
            exit(1);
        }
    }

    fclose(f);

    printf("Parent: %d child processes created\n", n);
    printf("Parent: Waiting for child processes to read child database\n");

    sleep(2);
    fflush(stdout);

    // initially all children are playing
    child_status = (bool *)malloc((n+1)*sizeof(bool));
    for(int i=0; i<n+1; i++) {
        child_status[i] = 1;
    }

    // initially the ball is thrown to the first child
    throw_to = 1;
    int pc = inPlay(child_status, n);

    formattedOutput(1);
    fflush(stdout);

    while(pc) {
        // throw the ball to the next child
        kill(childPIDs[throw_to], SIGUSR2);
        
        /* 
           wait for the child's signal signifying catch miss or catch success
           in case the child sends the signal before the pause function is called
           the received_signal flag will have been set to 1 and the control will flow 
           without hanging 
        */
       
        if(!received_signal) pause();
        else received_signal = 0;

        // fork a dummy child for allowing sequential status printing
        int dpid = fork();
        if(dpid) {
            FILE* fdc = fopen("dummycpid.txt", "w");
            
            fprintf(fdc, "%d\n", dpid);
            fflush(fdc);
            fclose(fdc);

            formattedOutput(0);
            printf("|    ");
            fflush(stdout);

            // intiate status printing by sending SIGUSR1 to the first child
            kill(childPIDs[1], SIGUSR1);
            waitpid(dpid, NULL, 0);

            printf("    |\n");
            fflush(stdout);
        }
        else {
            execlp("./dummy", "./dummy", NULL);
        }

        // check if the game is over
        pc = inPlay();
        // get the next child to throw the ball
        throw_to = getNextPlayingChild(throw_to);
    }

    formattedOutput(0);
    formattedOutput(1);
    fflush(stdout);

    // finally send SIGINT to all children to terminate them
    for(int i=1; i<=n; i++) {
        kill(childPIDs[i], SIGINT);
    }

    free(childPIDs);
    free(child_status);
    
    return 0;
}