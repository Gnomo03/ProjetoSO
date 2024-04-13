#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>

#include "../include/orchestrator.h"

char *output_folder;
int task_counter = 0;
Task *task_queue = NULL;

/*void process_status_request() {
    int fifo_fd = open("/tmp/server_fifo", O_WRONLY);
    if (fifo_fd < 0) {
        perror("Failed to open FIFO for writing status response");
        return;
    }

    char status_response[8192]; // Large buffer for detailed statuses
    int response_len = 0;
    Task *current = task_queue;

    response_len += snprintf(status_response + response_len, sizeof(status_response) - response_len, "Executing\n");
    while (current) {
        char task_details[2048];
        if (current->state == RUNNING) {
            snprintf(task_details, sizeof(task_details), "%d %s\n", current->task_id, current->command->args);
            strcat(status_response, task_details);
        }
        current = current->next;
    }

    strcat(status_response, "Scheduled\n");
    current = task_queue;
    while (current) {
        char task_details[1024];
        if (current->state == WAITING) {
            snprintf(task_details, sizeof(task_details), "%d %s\n", current->task_id, current->command->args);
            strcat(status_response, task_details);
        }
        current = current->next;
    }

    strcat(status_response, "Completed\n");
    current = task_queue;
    while (current) {
        char task_details[1024];
        if (current->state == COMPLETED) {
            snprintf(task_details, sizeof(task_details), "%d %s %ld ms\n", current->task_id, current->command->args, (long)(current->end_time - current->start_time));
            strcat(status_response, task_details);
        }
        current = current->next;
    }

    if (write(fifo_fd, status_response, strlen(status_response) + 1) < 0) {
        perror("Failed to write status response to FIFO");
    }

    close(fifo_fd);
}*/

void enqueue_task(Command *command)
{
    Task *new_task = malloc(sizeof(Task));
    if (new_task == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    new_task->command = command;
    new_task->next = NULL;

    if (task_queue == NULL)
    {
        task_queue = new_task;
    }
    else
    {
        Task *current = task_queue;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_task;
    }
}

Task *dequeue_task()
{
    if (task_queue == NULL)
    {
        return NULL;
    }
    else
    {
        Task *temp = task_queue;
        task_queue = task_queue->next;
        return temp;
    }
}

void process_command(Command *command)
{
    char output_path[MAX_SZ];
    time_t start_time, end_time;
    FILE *output_file;

    int task_id = ++task_counter;

    snprintf(output_path, MAX_SZ, "%s/execution_log_TASK%d.txt", output_folder, task_id);
    
    output_file = fopen(output_path, "a");
    if (output_file == NULL)
    {
        fprintf(stderr, "Failed to open output file\n");
        exit(EXIT_FAILURE);
    }

    start_time = time(NULL);

    pid_t pid = fork();
    if (pid == 0)
    {
        // Redirect stdout and stderr to the output file
        dup2(fileno(output_file), STDOUT_FILENO);
        dup2(fileno(output_file), STDERR_FILENO);

        // Prepare the arguments for execvp
        int argc = 0;
        char *args[256]; // adjust size as needed
        char *token = strtok(command->args, " ");
        while (token != NULL) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL; // null terminate the array of arguments

        // Execute the command
        execvp(args[0], args);

        // If execvp fails, it returns, and the error is handled below
        fprintf(stderr, "Failed to execute command\n");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
    }

    end_time = time(NULL);

    fprintf(output_file, "Task ID: %d, Execution Time: %ld seconds\n", task_id, (long)(end_time - start_time));

    fclose(output_file);
}

void parse_command(const char *input, Command *command)
{
    char cmd[MAX_SZ], args[MAX_SZ];
    int time;
    sscanf(input, "%s %d %s \"%[^\"]\"", cmd, &time, command->flag, args);

    if (strcmp(cmd, "execute") == 0)
        command->type = EXECUTE;
    else if (strcmp(cmd, "status") == 0)
        command->type = STATUS;

    command->time = time;
    strcpy(command->args, args);
}

void setup_fifo(const char *fifo_path)
{
    char buffer[MAX_SZ];
    int fifo_fd;

    mkfifo(fifo_path, 0666);
    fifo_fd = open(fifo_path, O_RDONLY);
    if (fifo_fd < 0)
    {
        perror("Failed to open FIFO");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        ssize_t num_bytes_read = read(fifo_fd, buffer, MAX_SZ);
        if (num_bytes_read > 0)
        {
            buffer[num_bytes_read] = '\0';
            Command command;
            parse_command(buffer, &command);

            if (command.type == EXECUTE)
            {
                Command *new_command = malloc(sizeof(Command));
                if (new_command == NULL)
                {
                    fprintf(stderr, "Memory allocation failed\n");
                    exit(EXIT_FAILURE);
                }
                memcpy(new_command, &command, sizeof(Command));
                enqueue_task(new_command);
            }
            else if (command.type == STATUS)
            {
                //process_status_request();
            }
        }

        Task *current_task = dequeue_task();
        if (current_task != NULL)
        {
            process_command(current_task->command);
            free(current_task->command);
            free(current_task);
        }
    }

    close(fifo_fd);
    unlink(fifo_path);
}

void ensure_output_folder_exists(const char *folder)
{
    struct stat st = {0};
    if (stat(folder, &st) == -1)
    {
        if (mkdir(folder, 0700) == -1)
        {
            fprintf(stderr, "Error creating directory '%s': %s\n", folder, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s output_folder\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    output_folder = argv[1];

    ensure_output_folder_exists(output_folder);

    const char *fifo_path = "/tmp/server_fifo";
    setup_fifo(fifo_path);

    return 0;
}
