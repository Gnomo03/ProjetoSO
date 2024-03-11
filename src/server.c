#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "../include/server.h"

#define MAX_SZ 1024

char *output_folder;
int parallel_tasks;

// Função para criar o diretório se ele não existir
void ensure_output_folder_exists(const char *folder) {
    struct stat st = {0};
    if (stat(folder, &st) == -1) {
        if (mkdir(folder, 0700) == -1) {
            fprintf(stderr, "Error creating directory '%s': %s\n", folder, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void execute_command(const char *cmd) {
    char buffer[MAX_SZ];
    FILE *fp = popen(cmd, "r");
    char output_path[MAX_SZ];
    
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    snprintf(output_path, MAX_SZ, "%s/output_%d_%ld.txt", output_folder, getpid(), time(NULL));

    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        printf("Failed to open output file\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
        fprintf(output_file, "%s", buffer);
    }

    fclose(output_file);
    pclose(fp);
}

void setup_fifo(const char *fifo_path) {
    char buffer[MAX_SZ];
    int fifo_fd;

    mkfifo(fifo_path, 0666);

    printf("Waiting for commands...\n");

    fifo_fd = open(fifo_path, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Failed to open FIFO");
        exit(1);
    }

    while (1) {
        ssize_t num_bytes_read = read(fifo_fd, buffer, MAX_SZ);
        if (num_bytes_read > 0) {
            buffer[num_bytes_read] = '\0';
            printf("Received: %s\n", buffer);

            if (strncmp(buffer, "execute", 7) == 0) {
                execute_command(buffer + 8);
            }
        }
    }

    close(fifo_fd);
    unlink(fifo_path);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s output_folder parallel-tasks sched-policy\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    output_folder = argv[1];
    parallel_tasks = atoi(argv[2]);
    const char *sched_policy = argv[3];

    ensure_output_folder_exists(output_folder);

    const char *fifo_path = "/tmp/server_fifo";
    setup_fifo(fifo_path);

    return 0;
}