#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../include/client.h"

void send_command_to_server(const char *command)
{
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0)
    {
        perror("Opening Error FIFO");
        exit(EXIT_FAILURE);
    }

    if (write(fifo_fd, command, strlen(command)) < 0)
    {
        perror("Writing Error FIFO");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }

    close(fifo_fd);
}

void receive_and_print_server_response() {
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Error opening FIFO for reading");
        exit(EXIT_FAILURE);
    }

    char response[8192]; // Increase buffer size if necessary
    ssize_t bytes_read = read(fifo_fd, response, sizeof(response) - 1);
    if (bytes_read > 0) {
        response[bytes_read] = '\0'; // Ensure null-termination
        printf("%s\n", response);
    } else {
        printf("No response received or error in reading FIFO.\n");
    }

    close(fifo_fd);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <command>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char command[BUFFER_SIZE];

    // Handle "execute" command
    if (strcmp(argv[1], "execute") == 0) {
        if (argc < 5) {
            printf("Usage: %s <command_type> <duration> -u \"program args\" or -p \"program1 | program2 | ...\"\n", argv[0]);
            return EXIT_FAILURE;
        }
        int duration = atoi(argv[2]);
        char *flag = argv[3];
        char *program = argv[4];

        if (strcmp(flag, "-u") == 0 || strcmp(flag, "-p") == 0) {
            snprintf(command, BUFFER_SIZE, "%s %d %s \"%s\"", argv[1], duration, flag, program);
            send_command_to_server(command);
            printf("TASK Received.\n");
            return EXIT_SUCCESS; // Make sure to exit after successfully handling the command
        } else {
            printf("Invalid flag. Use -u for individual program execution or -p for pipeline execution.\n");
            return EXIT_FAILURE;
        }
    }

    // Handle "status" command
    else if (strcmp(argv[1], "status") == 0) {
        if (argc != 2) {
            printf("Usage: %s status\n", argv[0]);
            return EXIT_FAILURE;
        }

        //snprintf(command, BUFFER_SIZE, "status");
        //send_command_to_server(command);
        //receive_and_print_server_response();  // Fetch and print the response
        //return EXIT_SUCCESS;
    }

    // If the command is neither "execute" nor "status"
    else {
        printf("Invalid command type. Use 'execute' or 'status'.\n");
        return EXIT_FAILURE;
    }
}
