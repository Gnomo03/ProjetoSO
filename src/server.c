#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>

#define MAX_SZ 1024

char *output_folder;

void ensure_output_folder_exists(const char *folder) {
    struct stat st = {0};
    if (stat(folder, &st) == -1) {
        if (mkdir(folder, 0700) == -1) {
            fprintf(stderr, "Error creating directory '%s': %s\n", folder, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void process_command(const char *cmd) {
    char *args[MAX_SZ];
    char output_path[MAX_SZ];
    snprintf(output_path, MAX_SZ, "%s/output_%d_%ld.txt", output_folder, getpid(), time(NULL));

    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        printf("Failed to open output file\n");
        exit(1);
    }

    char *clean_cmd = strstr(cmd, "execute ");
    if (clean_cmd) clean_cmd += strlen("execute ");
    else clean_cmd = cmd;

    int argCount = 0;
    char *token = strtok(clean_cmd, " ");
    while (token != NULL && argCount < MAX_SZ - 1) {
        args[argCount++] = token;
        token = strtok(NULL, " ");
    }
    args[argCount] = NULL;

    if (strcmp(args[0], "ls") == 0 || (strcmp(args[0], "cat") == 0 && argCount > 1)) {
        pid_t pid = fork();
        if (pid == 0) {
            dup2(fileno(output_file), STDOUT_FILENO);
            execvp(args[0], args);
            fprintf(stderr, "Failed to execute command\n");
            exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            fprintf(stderr, "Fork failed\n");
            exit(1);
        }
    } else if (strcmp(args[0], "cat") == 0 && argCount <= 1) {
        fprintf(output_file, "Error: 'cat' command requires at least one argument.\n");
    } else {
        fprintf(output_file, "Command '%s' not allowed or unknown.\n", args[0]);
    }

    fclose(output_file);
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
            process_command(buffer);
        }
    }
    close(fifo_fd);
    unlink(fifo_path);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s output_folder\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    output_folder = argv[1];

    ensure_output_folder_exists(output_folder);

    const char *fifo_path = "/tmp/server_fifo";
    setup_fifo(fifo_path);

    return 0;
}
