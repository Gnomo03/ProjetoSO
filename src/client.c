#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_PATH "/tmp/server_fifo"
#define BUFFER_SIZE 1024

void send_command_to_server(const char *command) {
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0) {
        perror("Opening Error FIFO");
        exit(EXIT_FAILURE);
    }

    if (write(fifo_fd, command, strlen(command)) < 0) {
        perror("Writing Error FIFO");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }

    close(fifo_fd);
}

int main() {
    char input[BUFFER_SIZE];

    while (1) {
        if (fgets(input, BUFFER_SIZE, stdin) != NULL) {
            input[strcspn(input, "\n")] = 0;

            if (strncmp(input, "echo", 4) == 0) {
                if (strlen(input) > 5) {
                    send_command_to_server(input);
                    printf("(Mensagem sent)\n");
                } else {
                    printf("(missing arguments - echo <arg>)\n");
                }
            } else if (strncmp(input, "execute", 7) == 0) {
                if (strlen(input) > 8) {
                    send_command_to_server(input);
                    printf("(command executed)\n");
                } else {
                    printf("(missing arguments - execute <command>)\n");
                }
            } else {
                printf("Command unknown. Use 'echo' or 'execute'.\n");
            }
        }
    }

    return EXIT_SUCCESS;
}
