#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "../include/client.h"

void send_command_to_server(const char *command)
{
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd < 0)
    {
        perror("Opening Error FIFO");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(fifo_fd, command, strlen(command));
    if (bytes_written < 0)
    {
        perror("Writing Error FIFO");
        close(fifo_fd);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Command sent successfully, bytes written: %zd\n", bytes_written);
    }

    close(fifo_fd);
}

void read_status()
{
    int fifo_fd = open(RESPONSE_FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0)
    {
        perror("Failed to open response FIFO for reading");
        exit(EXIT_FAILURE);
    }

    printf("Response FIFO opened for reading successfully.\n");
    printf("Waiting for data from server...\n");

    char buffer[2048];
    ssize_t bytes_read;
    char result[8192] = {0}; // Increase size as needed based on expected maximum data volume
    while ((bytes_read = read(fifo_fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0'; // Ensure string termination
        strcat(result, buffer);    // Concatenate each read to the result buffer
    }

    if (result[0] == '\0')
    {
        printf("No data received from the FIFO.\n");
    }
    else
    {
        printf("Received from FIFO:\n%s\n", result); // Print all received data at once
    }

    close(fifo_fd);
    printf("Response FIFO closed after reading.\n");
    printf("Data reception completed.\n");
    exit(EXIT_SUCCESS); // Exit after all data is read and processed
}

int main(int argc, char *argv[])
{
    if (strcmp(argv[1], "status") == 0)
    {
        send_command_to_server("status");
        read_status();
        return EXIT_SUCCESS;
    }

    if (argc < 2)
    {
        printf("Usage: %s <command_type> ['execute' <duration> -u|-p \"command\"] or ['status']\n", argv[0]);
        return EXIT_FAILURE;
    }

    char command[BUFFER_SIZE];

    if (strcmp(argv[1], "execute") == 0)
    {
        int duration = atoi(argv[2]);
        char *flag = argv[3];
        char *program = argv[4];

        if (strcmp(flag, "-u") == 0 || strcmp(flag, "-p") == 0)
        {
            // sleep(duration);
            snprintf(command, BUFFER_SIZE, "%s %d %s \"%s\"", argv[1], duration, flag, program);
            send_command_to_server(command);
        }
        else
        {
            printf("Invalid flag. Use -u for individual program execution or -p for pipeline execution.\n");
            return EXIT_FAILURE;
        }
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        if (argc != 2)
        {
            printf("Usage for status: %s status\n", argv[0]);
            return EXIT_FAILURE;
        }
        send_command_to_server("status");
        read_status();
    }
    else
    {
        printf("Invalid command type. Use 'execute' or 'status'.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}