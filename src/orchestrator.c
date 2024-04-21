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
Task *archive_queue = NULL;

void setup_response_fifo() {
    unlink(RESPONSE_FIFO_PATH);
    if (mkfifo(RESPONSE_FIFO_PATH, 0666) != 0) {
        perror("Failed to create RESPONSE_FIFO");
        exit(EXIT_FAILURE);
    }
}

void send_status_over_fifo() {
    int fd = open(RESPONSE_FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("Open RESPONSE_FIFO failed");
        return;
    }

    printf("RESPONSE_FIFO opened successfully for writing.\n");

    Task *current = archive_queue;
    if(current == NULL){
        printf("No tasks in the queue.\n");
        dprintf(fd, "No tasks in the queue.\n");
        close(fd);  // Close the FIFO here
        return;
    }
    while (current != NULL) {
        printf("Sending status: Task %d: %s\n", current->task_id, 
              (current->status == SCHEDULED) ? "Scheduled" :
              (current->status == EXECUTING) ? "Executing" :
              (current->status == COMPLETED) ? "Completed" : "Failed");
        dprintf(fd, "Task %d: %s\n", current->task_id, 
                                    (current->status == SCHEDULED) ? "Scheduled" : 
                                    (current->status == EXECUTING) ? "Executing" :
                                    (current->status == COMPLETED) ? "Completed" : "Failed");
        current = current->next;
    }

    close(fd);  // Ensure the FIFO is closed after all writes
}


void enqueue_task(Command *command) {
    // Create a new task for the processing queue
    Task *new_task = malloc(sizeof(Task));
    if (new_task == NULL) {
        fprintf(stderr, "Memory allocation failed for new task\n");
        exit(EXIT_FAILURE);
    }
    new_task->command = command;
    new_task->status = SCHEDULED;
    new_task->task_id = ++task_counter;
    new_task->next = NULL;

    // Enqueue in processing queue
    if (task_queue == NULL) {
        task_queue = new_task;
    } else {
        Task *current = task_queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_task;
    }

    // Create a separate task for the archive queue to maintain a history
    Task *archive_task = malloc(sizeof(Task));
    if (archive_task == NULL) {
        fprintf(stderr, "Memory allocation failed for archive task\n");
        exit(EXIT_FAILURE);
    }
    // Deep copy the task, need to ensure deep copy of command if necessary
    *archive_task = *new_task;
    archive_task->command = malloc(sizeof(Command));
    if (archive_task->command == NULL) {
        fprintf(stderr, "Memory allocation failed for command in archive task\n");
        exit(EXIT_FAILURE);
    }
    *archive_task->command = *command;  // Shallow copy is sufficient if command fields are simple types

    // Enqueue in archive queue
    if (archive_queue == NULL) {
        archive_queue = archive_task;
    } else {
        Task *current_archive = archive_queue;
        while (current_archive->next != NULL) {
            current_archive = current_archive->next;
        }
        current_archive->next = archive_task;
    }

    printf("Task ID %d enqueued: %s\n", new_task->task_id, command->args);
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

    int task_id = task_counter;

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
                printf("Status command received\n");
                send_status_over_fifo();

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

    setup_response_fifo();

    const char *fifo_path = "/tmp/server_fifo";
    setup_fifo(fifo_path);


    return 0;
}