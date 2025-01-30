#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "boardgen.c"

#define loop(n) for(int i=0; i<n; i++)

void help() {
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

int A[9][9], S[9][9];
int fds[9][2];
int neighbors[9][4]; // first 2 are row neighbors, next 2 are column neighbors

void init_neighbors(int fds[9][2]) {
    neighbors[0][0] =fds[1][1]; neighbors[0][1] =fds[2][1];
    neighbors[0][2] =fds[3][1]; neighbors[0][3] =fds[6][1];
    neighbors[1][0] =fds[0][1]; neighbors[1][1] =fds[2][1];
    neighbors[1][2] =fds[4][1]; neighbors[1][3] =fds[7][1];
    neighbors[2][0] =fds[0][1]; neighbors[2][1] =fds[1][1];
    neighbors[2][2] =fds[5][1]; neighbors[2][3] =fds[8][1];
    neighbors[3][0] =fds[4][1]; neighbors[3][1] =fds[5][1];
    neighbors[3][2] =fds[0][1]; neighbors[3][3] =fds[6][1];
    neighbors[4][0] =fds[3][1]; neighbors[4][1] =fds[5][1];
    neighbors[4][2] =fds[1][1]; neighbors[4][3] =fds[7][1];
    neighbors[5][0] =fds[3][1]; neighbors[5][1] =fds[4][1];
    neighbors[5][2] =fds[2][1]; neighbors[5][3] =fds[8][1];
    neighbors[6][0] =fds[7][1]; neighbors[6][1] =fds[8][1];
    neighbors[6][2] =fds[0][1]; neighbors[6][3] =fds[3][1];
    neighbors[7][0] =fds[6][1]; neighbors[7][1] =fds[8][1];
    neighbors[7][2] =fds[1][1]; neighbors[7][3] =fds[4][1];
    neighbors[8][0] =fds[6][1]; neighbors[8][1] =fds[7][1];
    neighbors[8][2] =fds[2][1]; neighbors[8][3] =fds[5][1];
}

void pass_message(char* message, int write_end) {
    int stdout_copy = dup(1);
    close(1);
    dup(write_end);
    printf("%s", message);
    close(1);
    dup(stdout_copy);
}

void pass_block_digits(int A[9][9], int no, char type) {
    char message[50];
    int temp[9];
    for(int j=0; j<3; j++) {
        for(int k=0; k<3; k++) {
            temp[3*j+k] = A[3*(no/3)+j][3*(no%3)+k];
        }
    }
    sprintf(message,"%c %d %d %d %d %d %d %d %d %d", type, temp[0], temp[1], temp[2], temp[3], temp[4], temp[5], temp[6], temp[7], temp[8]);
    pass_message(message, fds[no][1]);
}

void put_digit() {
    int b, c, d;
    scanf("%d %d %d", &b, &c, &d);
    char message[20];
    sprintf(message, "p %d %d", c, d);
    pass_message(message, fds[b][1]);
}

int main() {
    // 0- read, 1- write
    loop(9) {
        pipe(fds[i]);
    }

    init_neighbors(fds);
    
    loop(9) {
        int pid = fork();
        if(pid == 0) {
            
            char title[10], blockno[2], bfdin[15], bfdout[15], rn1fdout[15], rn2fdout[15], cn1fdout[15], cn2fdout[15], geometry[20];
            sprintf(title, "Block %d", i);
            sprintf(blockno, "%d", i);
            sprintf(bfdin, "%d", fds[i][0]);
            sprintf(bfdout, "%d", fds[i][1]);
            sprintf(rn1fdout, "%d", neighbors[i][0]);
            sprintf(rn2fdout, "%d", neighbors[i][1]);
            sprintf(cn1fdout, "%d", neighbors[i][2]);
            sprintf(cn2fdout, "%d", neighbors[i][3]);

            int x = 700 + (i%3)*275, y = 100 + (i/3)*275;
            sprintf(geometry, "17x8+%d+%d", x, y);
            execlp("xterm", "xterm", "-T", title, "-fa", "Monospace", "-fs", "15", "-geometry", geometry, "-bg", "#331100",
            "-e" ,"./block", blockno, bfdin, bfdout, rn1fdout, rn2fdout, cn1fdout, cn2fdout, NULL);
        }
    }
    
    help();

    while(1) {
        char c; 
        scanf("%c", &c);

        switch(c) {
            case 'n':
                newboard(A, S);
                loop(9) pass_block_digits(A, i, 'n');
                break;
            case 'p':
                put_digit();
                break;
            case 's':
                loop(9) pass_block_digits(S, i, 's');
                break;
            case 'h':
                help();
                break;
            case 'q':
                break;
        }
    }

    return 0;
}



