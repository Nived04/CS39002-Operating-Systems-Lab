#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define loop(i, n) for(int i=0; i<n; i++)

// variables for the blocks (initial and current)
int A[3][3], B[3][3];

// variables for read and write end of pipe with coordinator
// and write end of pipes with neighbors
int blockno, bfdin, bfdout, rn1fdout, rn2fdout, cn1fdout, cn2fdout;

// function to print the block in the given format
void print_block(int A[3][3]) {
    loop(i, 3) {
        printf(" +---+---+---+\n ");
        loop(j, 3) {
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

// function to fill the block with the digits sent by the coordinator
void fill_block() {
    loop(i, 3) {
        loop(j, 3) {
            scanf("%d", &A[i][j]);
            B[i][j] = A[i][j];
        }
    }
}

// function to pass the message via the write end of the required block's pipe
void pass_message(char* message, int write_end) {
    int stdout_copy = dup(1);
    close(1);
    dup(write_end);
    printf("%s\n", message);
    close(1);
    dup(stdout_copy);
}

// function to check if the digit d is already present in the block (ignore if 0)
bool block_conflict(int d) {
    loop(i, 9) {
        if(d!=0 && B[i/3][i%3] == d) return true;
    }
    return false;
}

// pass the location and the digit to the neighbors and get the conflict response
bool neighbor_conflict(char type, int loc, int n1, int n2, int d) {
    char pass[10]; 
    int response[2];

    sprintf(pass, "%c %d %d %d", type, loc, d, bfdout);
    pass_message(pass, n1);
    
    scanf("%d", &response[0]);
    // if the response is 0, then there is a conflict
    if(response[0] == 0) return true;
    
    pass_message(pass, n2);
    scanf("%d", &response[1]);
    
    if(response[1] == 0) return true;
    return false;
}

// function to put the digit d at the cell c of the block
void put_digit() {
    int c, d;
    scanf("%d %d", &c, &d);
    // if the cell is read-only, then give read-only conflict
    if(A[c/3][c%3] != 0) {
        printf("Read-only Cell\n");
        sleep(2);
        return;
    }
    // if the digit is not an empty space (or 0) check other conflicts
    if(d != 0)  {
        if(block_conflict(d)) {
            printf("Block conflict\n");
            sleep(2);
            return;
        }
        if(neighbor_conflict('r', c/3, rn1fdout, rn2fdout, d)) {
            printf("Row conflict\n");
            sleep(2);
            return;
        }
        if(neighbor_conflict('c', c%3, cn1fdout, cn2fdout, d)) {
            printf("Column conflict\n");
            sleep(2);
            return;
        }
    }
    // fill if no conflict or if the digit is 0 and not read-only cell
    B[c/3][c%3] = d;
}

// check the row or the column of the block for the digit d sent by the neighbor
void check_block_locationally(char type) {
    int loc, d, req_block_write; 
    scanf("%d %d %d", &loc, &d, &req_block_write);
    char response[5];
    int resp = 1;
    loop(i, 3) {
        if(d != 0) {
            if(type == 'r' && B[loc][i] == d) resp = 0;
            if(type == 'c' && B[i][loc] == d) resp = 0;
        }
    }

    sprintf(response, "%d", resp); 
    pass_message(response, req_block_write);   
} 

int main(int argc, char* argv[]) {
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

    while(1) {
        char c;
        scanf(" %c", &c);
        switch(c) {
            case 'n':
                fill_block();
                break;
            case 'p':
                put_digit();
                break;
            case 'r':
                check_block_locationally('r');
                break;
            case 'c':
                check_block_locationally('c');
                break;      
            case 'q':
                printf("Bye...\n");
                sleep(2);
                exit(0);
            default:
                break;
        }
        // print the current state of the block after every command
        print_block(B);
    }

    return 0;
}