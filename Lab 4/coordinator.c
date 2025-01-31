#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "boardgen.c"

#define loop(n) for(int i=0; i<n; i++)

// printing the help message
void print_help() {
    printf("Commands supported\n");
    printf("\tn\t\tStart new game\n");
    printf("\tp b c d\t\tPut digit d [1-9] at cell c [0-8] of block b [0-8]\n");  
    printf("\ts\t\tShow solution\n");
    printf("\th\t\tPrint the help message\n");
    printf("\tq\t\tQuit\n");

    printf("Numbering scheme for the blocks and cells\n");
    for(int i=0; i<9; i+=3) {
        printf("\t+---+---+---+\n");
        printf("\t| %d | %d | %d |\n", i, i+1, i+2);
    }
    printf("\t+---+---+---+\n");

}

// blocks for new board and solution
int A[9][9], S[9][9];
// file descriptors for pipes with the 9 child blocks
int fds[9][2];
// first 2 are row neighbors, next 2 are column neighbors
int neighbors[9][4]; 

// initialize the neighbors array
void init_neighbors(int fds[9][2]) {
    neighbors[0][0] = fds[1][1]; neighbors[0][1] = fds[2][1];
    neighbors[0][2] = fds[3][1]; neighbors[0][3] = fds[6][1];
    neighbors[1][0] = fds[0][1]; neighbors[1][1] = fds[2][1];
    neighbors[1][2] = fds[4][1]; neighbors[1][3] = fds[7][1];
    neighbors[2][0] = fds[0][1]; neighbors[2][1] = fds[1][1];
    neighbors[2][2] = fds[5][1]; neighbors[2][3] = fds[8][1];
    neighbors[3][0] = fds[4][1]; neighbors[3][1] = fds[5][1];
    neighbors[3][2] = fds[0][1]; neighbors[3][3] = fds[6][1];
    neighbors[4][0] = fds[3][1]; neighbors[4][1] = fds[5][1];
    neighbors[4][2] = fds[1][1]; neighbors[4][3] = fds[7][1];
    neighbors[5][0] = fds[3][1]; neighbors[5][1] = fds[4][1];
    neighbors[5][2] = fds[2][1]; neighbors[5][3] = fds[8][1];
    neighbors[6][0] = fds[7][1]; neighbors[6][1] = fds[8][1];
    neighbors[6][2] = fds[0][1]; neighbors[6][3] = fds[3][1];
    neighbors[7][0] = fds[6][1]; neighbors[7][1] = fds[8][1];
    neighbors[7][2] = fds[1][1]; neighbors[7][3] = fds[4][1];
    neighbors[8][0] = fds[6][1]; neighbors[8][1] = fds[7][1];
    neighbors[8][2] = fds[2][1]; neighbors[8][3] = fds[5][1];
}

// pass the message via the write end of the required block's pipe 
void pass_message(char* message, int write_end) {
    int stdout_copy = dup(1);
    close(1);
    dup(write_end);
    printf("%s\n", message);
    close(1);
    dup(stdout_copy);
}

// pass the digits of the block to the required block
void pass_block_digits(int A[9][9], int no) {
    char message[50];
    int temp[9];
    for(int j=0; j<3; j++) {
        for(int k=0; k<3; k++) {
            temp[3*j+k] = A[3*(no/3)+j][3*(no%3)+k];
        }
    }
    sprintf(message,"n %d %d %d %d %d %d %d %d %d", temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7], temp[8]);
    pass_message(message, fds[no][1]);
}

// pass the message to put the digit d at the cell c of the block b
void put_digit() {
    int b, c, d;
    scanf("%d %d %d", &b, &c, &d);
    char message[20];
    sprintf(message, "p %d %d", c, d);
    pass_message(message, fds[b][1]);
}

int main() {
    // 0 - read end, 1 - write end
    loop(9) {
        pipe(fds[i]);
    }

    init_neighbors(fds);
    
    loop(9) {
        int pid = fork();
        if(pid == 0) {   
            char title[10], blockno[2], bfdin[15], bfdout[15], geometry[20];
            // pipe write_end fd: first 2 are row neighbors, next 2 are column neighbors
            char write_ends[4][15]; 
            sprintf(title, "Block %d", i);
            sprintf(blockno, "%d", i);
            sprintf(bfdin, "%d", fds[i][0]);
            sprintf(bfdout, "%d", fds[i][1]);
            for(int j=0; j<4; j++) {
                sprintf(write_ends[j], "%d", neighbors[i][j]);
            }

            int x = 900 + (i%3)*270, y = 100 + (i/3)*290;
            sprintf(geometry, "17x8+%d+%d", x, y);

            execlp("xterm", "xterm", "-T", title, "-fa", "Monospace", "-fs", "15", "-geometry", geometry, "-bg", "#331100",
            "-e" ,"./block", blockno, bfdin, bfdout, write_ends[0], write_ends[1], write_ends[2], write_ends[3], NULL);
        }
    }
    
    print_help();

    bool started = false;

    while(1) {
        printf("Foodoku> ");
        char c; 
        scanf(" %c", &c);
        switch(c) {
            case 'n':
                started = true;
                newboard(A, S);
                loop(9) pass_block_digits(A, i);
                break;

            case 'p':
                if(!started) printf("Start a new game first\n");
                else put_digit();
                break;

            case 's':
                if(!started) printf("Start a new game first\n");
                else { loop(9) pass_block_digits(S, i); }
                break;

            case 'h':
                print_help();
                break;

            case 'q':
                // send the quit message to each child
                loop(9) pass_message("q", fds[i][1]);
                // the child will become zombies initially, but we will now call wait 9 times
                loop(9) wait(NULL);
                exit(0);

            default:
                printf("Invalid command\n");
                break;
        }
    }

    return 0;
}



