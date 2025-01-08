#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

const int MAX_DEP = 500;
const char* dag_file = "foodep.txt";
const char* visited_file = "done.txt";

// function to retrieve the dependencies of a foodule and return the total number of dependencies
int get_dependencies(FILE* f, int foodule, int dependencies[]) {
    // jump to the line that contains the dependencies of the argument foodule
    int cnt = 0;
    while(cnt != foodule) {
        if(fgetc(f) == '\n')
            cnt++;
    }
    while(fgetc(f) != ':') {
        continue;
    }

    char line[MAX_DEP];
    fgets(line, MAX_DEP, f);
    // printf("Line: %s", line);    

    // after the colon, there will either be a space (if there are some dependencies) or newline/space (if no dependencies)
    if(line[1] < 48 || line[1] > 57) {
        return 0;
    }

    // separate the dependencies and add them to the dependencies array for the current foodule
    int i=0;
    char *dep = strtok(line, " ");
    while(dep != NULL) {
        int val = atoi(dep);
        dependencies[i] = val;
        dep = strtok(NULL, " ");
        i++;
    }

    // return the number of dependencies
    return i;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Please enter a foodule to rebuild\n");
        exit(0);
    }

    FILE* fdag = fopen(dag_file, "r");
    // first line of the foodep file contains the total number of foodules
    int total_foodules;
    fscanf(fdag, "%d", &total_foodules);
    
    // if there is just 1 other argument, it means that it is the root process
    if(argc == 2) {
        FILE *fvf = fopen(visited_file, "w");
        for(int i=0; i<total_foodules; i++) {
            fputc('0', fvf);
        }
        fclose(fvf);
    }

    int foodule = atoi(argv[1]);

    if(foodule > total_foodules) {
        printf("Invalid foodule number\n");
        exit(0);
    }

    int* dependencies = (int*)malloc(MAX_DEP*sizeof(int));
    int tot_dep = get_dependencies(fdag, foodule, dependencies);
    fclose(fdag);

    // tranversing the dependencies of current foodule and rebuilding them
    for(int i=0; i<tot_dep; i++) {
        int child = dependencies[i];
        
        // read the "visited" file to retrieve the isVisited status
        FILE *fvf = fopen(visited_file, "r");
        char line[total_foodules + 1];
        fgets(line, sizeof(line), fvf);
        fclose(fvf);

        // do not fork if the child has already been visited
        if(line[child - 1] == '1') {
            continue;
        }    
        else {
            int pid = fork(); // signifies recursion of DFS
            if(pid == 0) {
                char child_num[10];
                sprintf(child_num, "%d", child);
                // calling execlp to create a new process with -child as additional argument that tells that it is not root
                execlp("./rebuild", "./rebuild", child_num, "-child", NULL);
            }
            else {
                wait(NULL);
            }
        }
    }

    // retrieve the visited status
    FILE* fvf = fopen(visited_file, "r");
    char line[total_foodules + 1];
    fgets(line, sizeof(line), fvf);

    // update the visited status of the current foodule
    line[foodule - 1] = '1';
    fclose(fvf);

    // write the updated visited status back to the file
    fvf = fopen(visited_file, "w");
    fputs(line, fvf);
    fclose(fvf);

    // print required message
    char buffer[100];
    sprintf(buffer, "foo%d rebuilt", foodule);
    printf("%s", buffer);

    if(tot_dep != 0) {
        printf(" from ");
        for(int i=0; i<tot_dep; i++) {
            char child_name[10];
            sprintf(child_name, "foo%d", dependencies[i]);
            printf("%s", child_name);
            if(i != tot_dep - 1) {
                printf(", ");
            }
        }
    }
    printf("\n");

    return 0;
}