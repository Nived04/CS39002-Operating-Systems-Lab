#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

int file_count = 0;

// a hash array that stores the owner name of the uid
char uid_to_name[1000000][32];

void read_etcpwd() {
    FILE *fp = fopen("/etc/passwd", "r");

    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *name = strtok(line, ":");
        // printf("name: %s\n", name);
        char *x = strtok(NULL, ":");
        // printf("x: %s\n", x);
        char *uid_str = strtok(NULL, ":");
        int uid = atoi(uid_str);
        // printf("uid_str: %s\n", uid_str);
        sprintf(uid_to_name[uid], "%s", name);
    }

    fclose(fp);
}

void list_files(char dir_name[], char ext[]) {
    struct dirent *reader;
    DIR *pDir;

    pDir = opendir(dir_name);
    if (pDir == NULL) {
        printf ("*** Cannot open directory '%s'\n", dir_name);
        return;
    }

    while((reader = readdir(pDir)) != NULL) {
        if(reader->d_type == DT_REG) {
            char *dot = strrchr(reader->d_name, '.');
            if(dot && strcmp(dot + 1, ext) == 0) {
                struct stat file_stat;
                char file_path[512];
                sprintf(file_path, "%s/%s", dir_name, reader->d_name);
                if(stat(file_path, &file_stat) == -1) {
                    perror("stat");
                    continue;
                }

                file_count++;
                printf("%-8d: %-16s %-16ld %s/%s\n", file_count, uid_to_name[file_stat.st_uid], file_stat.st_size, dir_name, reader->d_name);
            }
        }
        else if(reader->d_type == DT_DIR) {
            if(strcmp(reader->d_name, ".") != 0 && strcmp(reader->d_name, "..") != 0) {
                char new_dir[257];
                sprintf(new_dir, "%s/%s", dir_name, reader->d_name);
                list_files(new_dir, ext);
            }
        }
    }

    closedir (pDir);
}

int main (int argc, char *argv[]) {
    struct dirent *reader;
    DIR *pDir;

    read_etcpwd();

    if (argc != 3) {
        printf ("Usage: findall <dirname> <extension w/o .>\n");
        return 1;
    }

    pDir = opendir(argv[1]);
    if (pDir == NULL) {
        printf ("Cannot open directory '%s'\n", argv[1]);
        return 1;
    }

    char *extension = argv[2];

    printf("%-8s: %-16s %-16s %s\n", "NO", "OWNER", "SIZE", "NAME");
    printf("%-8s: %-16s %-16s %s\n", "--", "-----", "----", "----");

    list_files(argv[1], extension);

    printf("+++ %d files match the extension %s\n", file_count, extension);

    closedir (pDir);
    return 0;
}