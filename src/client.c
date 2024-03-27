#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_PATH "/home/sensei/Documents/GitHub/ProjetoSO/tmp/server_fifo"
#define BUFFER_SIZE 1024

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

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        printf("Usage: %s <command_type> <duration> -u \"program args\" or -p \"program1 | program2 | ...\"\n", argv[0]);
        return EXIT_FAILURE;
    }

    static int task_id = 1;

    char command[BUFFER_SIZE];

    if (strcmp(argv[1], "execute") == 0)
    {
        int duration = atoi(argv[2]);
        char *flag = argv[3];
        char *program = argv[4];

        if (strcmp(flag, "-u") == 0 || strcmp(flag, "-p") == 0)
        {
            snprintf(command, BUFFER_SIZE, "%s %d %s \"%s\"", argv[1], duration, flag, program);
            send_command_to_server(command);
            printf("TASK %d Received.\n", task_id++);
        }
        else
        {
            printf("Invalid flag. Use -u for individual program execution or -p for pipeline execution.\n");
            return EXIT_FAILURE;
        }
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        send_command_to_server(argv[1]);
        printf("Status queried.\n");
    }
    else
    {
        printf("Invalid command type. Use 'execute' or 'status'.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
