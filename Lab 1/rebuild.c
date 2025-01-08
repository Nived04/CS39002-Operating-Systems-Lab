#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

const int MAX_DEP = 100;

int get_dependencies(const char* file, int foodule, int dependencies[]) {
    FILE* f;
    f = fopen(file, "r");
    int cnt = 0;
    while(cnt != foodule) {
        if(fgetc(f) == '\n')
            cnt++;
    }
    while(fgetc(f) != ':') {
        continue;
    }

    char line[MAX_DEP];
    fgets(line, sizeof(line), f);
    // printf("Line: %s", line);    

    fclose(f);
    if(line[1] < 48 || line[1] > 57) {
        return 0;
    }

    int i=0;
    char *dep = strtok(line, " ");
    while(dep != NULL) {
        int val = atoi(dep);
        dependencies[i] = val;
        dep = strtok(NULL, " ");
        i++;
    }
    return i;
}

void rebuild_foodule(int node) {}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Please enter a foodule to rebuild\n");
        exit(1);
    }

    char* dag_file = "foodep.txt";
    char* visited_file = "done.txt";

    FILE* fdag = fopen(dag_file, "r");
    int total_foodules;
    fscanf(fdag, "%d", &total_foodules);
    
    if(argc == 2) {
        FILE *fvf = fopen(visited_file, "w");
        char* vis = (char*)malloc((total_foodules + 1)*sizeof(char));
        for(int i=0; i<total_foodules; i++) {
            vis[i] = '0';
        }
        fputs(vis, fvf);
        fclose(fvf);
    }

    fclose(fdag);
    int foodule = atoi(argv[1]);

    int* dependencies = (int*)malloc(MAX_DEP*sizeof(int));

    int tot_dep = get_dependencies(dag_file, foodule, dependencies);

    // printf("%d\n", tot_dep);

    for(int i=0; i<tot_dep; i++) {
        int child = dependencies[i];
        FILE *fvf = fopen(visited_file, "r");
        char line[total_foodules + 1];
        fgets(line, sizeof(line), fvf);
        fclose(fvf);

        if(line[child - 1] == '1') {
            continue;
        }    
        else {
            int pid = fork();
            if(pid == 0) {
                char child_num[10];
                sprintf(child_num, "%d", child);
                execlp("./rebuild", "./rebuild", child_num, "-child", NULL);
            }
            else {
                wait(NULL);
            }
        }
    }

    FILE* fvf = fopen(visited_file, "r");
    char line[total_foodules + 1];
    fgets(line, sizeof(line), fvf);
    line[foodule - 1] = '1';
    fclose(fvf);

    fvf = fopen(visited_file, "w");
    fputs(line, fvf);
    fclose(fvf);

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