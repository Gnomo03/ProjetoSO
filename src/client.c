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
    if (argc < 3)
    {
        printf("Usage: %s <command_type> <command>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char command[BUFFER_SIZE];

    if (strcmp(argv[1], "echo") == 0 || strcmp(argv[1], "execute") == 0)
    {
        snprintf(command, BUFFER_SIZE, "%s %s", argv[1], argv[2]);
        send_command_to_server(command);

        if (strcmp(argv[1], "echo") == 0)
        {
            printf("(Message sent)\n");
        }
        else if (strcmp(argv[1], "execute") == 0)
        {
            printf("(Command executed)\n");
        }
    }
    else
    {
        printf("Command type must be 'echo' or 'execute'\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
