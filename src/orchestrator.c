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
#include <sys/select.h>

char *output_folder;
int task_counter = 0;
int max_parallel_tasks = 0;
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

    Task *current = archive_queue;  // Ensure this points to the archive_queue
    if (current == NULL) {
        write(fd, "No tasks in the queue.\n", 22);
        close(fd);
        return;
    }
    while (current != NULL) {
        //printf("Sending status: Task %d: %s\n", current->task_id,
        //       (current->status == SCHEDULED) ? "Scheduled" :
        //       (current->status == EXECUTING) ? "Executing" :
        //       (current->status == COMPLETED) ? "Completed" : "Failed");
        dprintf(fd, "Task %d: %s\n", current->task_id,
               (current->status == SCHEDULED) ? "Scheduled" :
               (current->status == EXECUTING) ? "Executing" :
               (current->status == COMPLETED) ? "Completed" : "Failed");
        current = current->next;
    }
    close(fd);  // Close the FIFO after all writes
}

void enqueue_task(Task *new_task) {
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
    *archive_task = *new_task;  // Shallow copy of the task structure

    archive_task->command = malloc(sizeof(Command));
    if (archive_task->command == NULL) {
        fprintf(stderr, "Memory allocation failed for command in archive task\n");
        exit(EXIT_FAILURE);
    }
    *archive_task->command = *new_task->command;  // Deep copy of the command

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

    //printf("Task ID %d enqueued: %s\n", new_task->task_id, new_task->command->args);
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


void update_archive_pid(int task_id, int pid) {
    Task *current = archive_queue;
    while (current != NULL) {
        if (current->task_id == task_id) {
            current->task_pid = pid;
            break;
        }
        current = current->next;
    }
}

void update_archive_task_status(int task_id, TaskStatus new_status) {
    Task *current = archive_queue;
    while (current != NULL) {
        if (current->task_id == task_id) {
            current->status = new_status;
            break;
        }
        current = current->next;
    }
}

void process_command(Task *task) {
    if (task == NULL) return;

    char output_path[MAX_SZ];
    FILE *output_file;

    // Construct output file path for the task logs
    snprintf(output_path, MAX_SZ, "%s/execution_log_TASK%d.txt", output_folder, task->task_id);
    output_file = fopen(output_path, "a");
    if (output_file == NULL) {
        fprintf(stderr, "Failed to open output file\n");
        exit(EXIT_FAILURE);
    }

    // Update status to EXECUTING
    task->start_time = time(NULL);
    task->status = EXECUTING; // Update status in task_queue
    update_archive_task_status(task->task_id, EXECUTING); // Synchronize status in archive_queue

    pid_t pid = fork();
    if (pid == 0) {
        // Child process: Redirect stdout and stderr to output file
        dup2(fileno(output_file), STDOUT_FILENO);
        dup2(fileno(output_file), STDERR_FILENO);

        // Parse command arguments
        char *args[256];
        int argc = 0;
        char *token = strtok(task->command->args, " ");
        while (token != NULL) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;  // Null-terminate the args array

        printf("Running command\n");
        // TODO: Retirar
        sleep(10);

        // Execute the command
        execvp(args[0], args);

        // If execvp fails, print error and exit child process
        fprintf(stderr, "Failed to execute command\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process: Wait for the child to finish
        task->task_pid = pid;
        update_archive_pid( task->task_id, pid );
        //
    } else {
        // Fork failed
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
    }

    // Log the task execution time and final status
    fprintf(output_file, "Task ID: %d, Execution Time: %ld seconds\n", task->task_id, (long)(task->execution_time));
    fclose(output_file);
}

void check_all_status(){
    Task *current = archive_queue;
    while (current != NULL) {
        check_child_status(current);
        current = current->next;
    }
}

void check_child_status( Task *task ){
    int status = 0 ;
    if( task->task_pid != 0 ){
        int result = waitpid( task->task_pid, &status, WNOHANG );
        if (result == 0) {
            // Child is still running
            //printf("Child is still running\n");
        } else if (result == -1) {
            task->end_time = time(NULL);
            task->execution_time = task->end_time - task->start_time;
            task->status = FAILED;
            task->task_pid = 0;
            update_archive_task_status(task->task_id, task->status); // Synchronize status in archive_queue
        } else {
            // Child has terminated
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0 ) {
                task->status = COMPLETED;
                task->end_time = time(NULL);
                task->execution_time = task->end_time - task->start_time;
                task->task_pid = 0;
                update_archive_task_status(task->task_id, task->status); // Synchronize status in archive_queue
            }
        }    
    }
}

void parse_command(const char *input, Command *command){
    char cmd[MAX_SZ], args[MAX_SZ];
    int time;
    sscanf(input, "%s %d %s \"%[^\"]\"", cmd, &time, command->flag, args);

    if (strcmp(cmd, "execute") == 0){
        if (strcmp(command->flag, "-u") == 0){
            command->type = EXECUTE; 
        }
        else if (strcmp(command->flag, "-p") == 0){
            // Conta o número de comandos separados por '|'
            int command_count = 1;
            for (char *p = args; *p; p++) {
                if (*p == '|') command_count++;
            }
            if (command_count > max_parallel_tasks) {
                printf("Error: Number of commands in pipeline exceeds maximum allowed (%d).\n", max_parallel_tasks);
            } else {
                command->type = EXECUTE;
            }
        }   
    }
    else if (strcmp(cmd, "status") == 0)
        command->type = STATUS;

    command->time = time;
    strcpy(command->args, args);
}

void setup_fifo(const char *fifo_path)
{
    char buffer[MAX_SZ];
    int fifo_fd;

    // Create FIFO file with appropriate permissions
    if (mkfifo(fifo_path, 0666) != 0 && errno != EEXIST) {
        perror("Failed to create FIFO");
        exit(EXIT_FAILURE);
    }

    // Open FIFO for reading
    fifo_fd = open(fifo_path, O_RDONLY);
    if (fifo_fd < 0)
    {
        perror("Failed to open FIFO");
        exit(EXIT_FAILURE);
    }

    while(1){
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

                // Create and enqueue a new task based on the command
                Task *new_task = malloc(sizeof(Task));
                if (new_task == NULL) {
                    fprintf(stderr, "Memory allocation failed for new task\n");
                    free(new_command);  // Free previously allocated memory
                    exit(EXIT_FAILURE);
                }
                new_task->command = new_command;
                new_task->task_id = ++task_counter;
                new_task->status = SCHEDULED;
                new_task->next = NULL;
                enqueue_task(new_task);
            }
            else if (command.type == STATUS)
            {
                send_status_over_fifo();
            }
        }
        // Process each task as it is dequeued
        Task *current_task = dequeue_task();
        if (current_task != NULL)
        {
            process_command(current_task);
            // TODO: Limpar da memória quando a tarefa termina
            //free(current_task->command);
            //free(current_task);
            //
        }
        check_all_status();
        //
        struct timespec req, rem;
        req.tv_sec = 0;                  // Seconds
        req.tv_nsec = 1000 * 1000 * 100; // 100 milliseconds to nanoseconds
        nanosleep(&req, &rem);
    }

    // TODO: Free memory from task
    // Cleanup: Close and unlink the FIFO
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
            perror("Error creating directory");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]){
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s output_folder\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    output_folder = argv[1];
    max_parallel_tasks = atoi(argv[2]);  // Converte a string para inteiro
    // char *sched_policy = argv[3];

    ensure_output_folder_exists(output_folder);

    setup_response_fifo();

    const char *fifo_path = "/tmp/server_fifo";
    setup_fifo(fifo_path);


    return 0;
}