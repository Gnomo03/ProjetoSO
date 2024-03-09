#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/server.h"

#define BUFFER_SIZE 1024

void execute_command(const char *cmd) {
    char buffer[BUFFER_SIZE];
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
        printf("%s", buffer);
    }

    pclose(fp);
}

void setup_fifo(const char *fifo_path) {
    char buffer[BUFFER_SIZE];
    int fifo_fd;

    mkfifo(fifo_path, 0666);

    printf("Waiting for commands...\n");

    fifo_fd = open(fifo_path, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Failed to open FIFO");
        exit(1);
    }

    while (1) {
        ssize_t num_bytes_read = read(fifo_fd, buffer, BUFFER_SIZE);
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

int main() {
    const char *fifo_path = "/tmp/server_fifo";
    setup_fifo(fifo_path);

    return 0;
}
