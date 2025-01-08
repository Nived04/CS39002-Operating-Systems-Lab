#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>

int main() {
    int id = fork();
    if(id == 0) {
        printf("Child process\n");
        execlp("cal","cal","2025",NULL);
    }
    else {
        wait(NULL);
        printf("\n--------------------------\nParent process\n");
    }
    return 0;
}