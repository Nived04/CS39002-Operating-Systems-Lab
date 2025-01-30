#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

int stdout_copy;
int blockno, bfdin, bfdout, rn1fdout, rn2fdout, cn1fdout, cn2fdout;

void print_block(int A[3][3]) {
    for(int i=0; i<3; i++) {
        printf(" +---+---+---+\n ");
        for(int j=0; j<3; j++) {
            if(A[i][j] == 0) {
                printf("|   ");
            } else {
                printf("| %d ", A[i][j]);
            }
        }
        printf("|\n");
    }
    printf(" +---+---+---+\n");
}


void initialize_block(int A[3][3], int B[3][3]) {
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            scanf("%d", &A[i][j]);
            B[i][j] = A[i][j];
        }
    }
    print_block(B);
}

void pass_message(char* message, int write_end) {
    close(1);
    dup(write_end);
    printf("%s\n", message);
    close(1);
    dup(stdout_copy);
}

bool block_conflict(int B[3][3], int d) {
    for(int i=0; i<9; i++) {
        if(B[i/3][i%3] == d) return true;
    }
    return false;
}

bool neighbor_conflict(char type, int loc, int n1, int n2, int b, int d) {
    char pass[10]; 
    sprintf(pass, "%c %d %d %d", type, loc, d, b);
    pass_message(pass, n1);
    int response;
    scanf("%d", &response);
    if(response == 0) return true;
    pass_message(pass, n2);
    scanf("%d", &response);
    if(response == 0) return true;
    return false;
}

void put_digit(int A[3][3], int B[3][3]) {
    int c, d;
    scanf("%d %d", &c, &d);
    if(A[c/3][c%3] != 0) {
        // print_block(B);
        printf("Read-only Cell\n");
        fflush(NULL);
        sleep(3);
        return;
    }
    if(block_conflict(B, d)) {
        printf("Block conflict\n");
        fflush(NULL);
        sleep(3);
        return;
    }
    if(neighbor_conflict('r', c/3, rn1fdout, rn2fdout, bfdout, d)) {
        printf("Row conflict\n");
        fflush(NULL);
        sleep(3);
        return;
    }
    if(neighbor_conflict('c', c%3, cn1fdout, cn2fdout, bfdout, d)) {
        printf("Column conflict\n");
        fflush(NULL);
        sleep(3);
        return;
    }
    B[c/3][c%3] = d;
    print_block(B);
}

void check_block(char type, int B[3][3]) {
    int loc, d, req_block_write; 
    scanf("%d %d %d", &loc, &d, &req_block_write);
    char response[5];
    int resp = 1;
    for(int i=0; i<3; i++) {
        if(type == 'r') {
            if(B[loc][i] == d) resp = 0;
        }
        else {
            if(B[i][loc] == d) resp = 0;
        }
    }
    sprintf(response, "%d", resp); 
    pass_message(response, req_block_write);   
} 

void print_solution() {
    int temp[3][3];
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            scanf("%d", &temp[i][j]);
        }
    }
    print_block(temp);
}

int main(int argc, char* argv[]) {
    stdout_copy = dup(1);

    // permanently close the stdin file descriptor
    close(0);

    blockno = atoi(argv[1]);
    bfdin = atoi(argv[2]); // read port of pipe
    bfdout = atoi(argv[3]); // write port of pipe
    // write port of neighbors
    rn1fdout = atoi(argv[4]); 
    rn2fdout = atoi(argv[5]);
    cn1fdout = atoi(argv[6]);
    cn2fdout = atoi(argv[7]);

    dup(bfdin);

    printf("Block %d ready\n", blockno);
    int A[3][3], B[3][3];
    while(1) {
        char c;
        scanf("%c", &c);
        switch(c) {
        case 'n':
            initialize_block(A, B);
            break;
        case 'p':
            put_digit(A, B);
            break;
        case 'r':
            check_block('r', B);
            break;
        case 'c':
            check_block('c', B);
            break;
        case 's':
            print_solution();
            break;       
        case 'q':
            printf("Bye...");
            sleep(3);
            exit(0);
            break;
        default:
            break;
        }
    }
}