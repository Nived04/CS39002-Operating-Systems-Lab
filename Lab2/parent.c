#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>

int throw_to, *childPIDs, n;
bool *child_status;

bool inPlay() {
    int cnt = 0, i;
    for(i=1; i<=n; i++) {
        if(child_status[i] == 1) cnt++;
    }
    return (cnt == 1) ? 0 : 1;
}

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

void formattedOutput(int bef) {
    char s1[10 + 8*n], s2[11+8*n], temp[10];
 
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

void signalHandler(int sig) {
    if(sig == SIGUSR2) child_status[throw_to] = 0; // out of the game
    return;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("Please enter a single argument denoting number of children\n");
        exit(1);
    }

    n = atoi(argv[1]);
    
    signal(SIGUSR1, signalHandler);
    signal(SIGUSR2, signalHandler);

    childPIDs = (int *)malloc((n+1)*sizeof(int));

    FILE *f = fopen("childpid.txt", "w");

    fprintf(f, "%d\n", n);
    fflush(f);

    for(int i=0; i<n; i++) {
        int cpid = fork();

        if(cpid) {
            childPIDs[i+1] = cpid;
            fprintf(f, "%d\n", cpid);
            fflush(f);
        }
        else {
            char temp[12];
            sprintf(temp, "%d", i+1);
            execlp("./child", "./child", NULL);
            perror("execlp");
            exit(1);
        }
    }

    fclose(f);
    printf("Parent: %d child processes created\n", n);
    printf("Parent: Waiting for child processes to read child database\n");
    sleep(2);
    fflush(stdout);

    child_status = (bool *)malloc((n+1)*sizeof(bool));
    for(int i=0; i<n+1; i++) {
        child_status[i] = 1;
    }

    throw_to = 1;
    int pc = inPlay(child_status, n);

    formattedOutput(1);
    fflush(stdout);

    while(pc) {
        kill(childPIDs[throw_to], SIGUSR2);
        pause();

        int dpid = fork();
        if(dpid) {
            FILE* fdc = fopen("dummycpid.txt", "w");
            
            fprintf(fdc, "%d\n", dpid);
            fflush(fdc);
            fclose(fdc);

            formattedOutput(0);
            printf("|    ");
            fflush(stdout);

            kill(childPIDs[1], SIGUSR1);
            waitpid(dpid, NULL, 0);

            printf("    |\n");
            fflush(stdout);
        }
        else {
            execlp("./dummy", "./dummy", NULL);
        }

        pc = inPlay();
        throw_to = getNextPlayingChild(throw_to);
    }

    formattedOutput(0);
    formattedOutput(1);
    fflush(stdout);

    for(int i=1; i<=n; i++) {
        kill(childPIDs[i], SIGINT);
    }

    free(childPIDs);
    free(child_status);
    
    return 0;
}