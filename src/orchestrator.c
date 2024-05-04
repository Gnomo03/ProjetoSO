#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "orchestrator.h"
#include "queue.h"
#include "task.h"

Task *task_queue = NULL;
Task *archive_queue = NULL;

char *output_folder;
int task_counter = 0;
int max_parallel_tasks = 0;


/// @brief Update the status status of a task in the archive_queue
void update_task_status(){
    Task *current = archive_queue;
    while (current != NULL) {
        if (current->status == EXECUTING) {
            int status = 0;
            int result = waitpid(current->task_pid, &status, WNOHANG);
            if (result == 0) {
                // Child is still running
            } 
            else {
                if (result == -1) {
                    current->end_time = time(NULL);
                    current->execution_time = current->end_time - current->start_time;
                    current->task_pid = 0;
                    current->status = FAILED;
                    printf("Task %d failed; total time=%ld\n", current->task_id, current->execution_time);
                } 
                else {
                    // Child has terminated
                    if ( WIFEXITED(status) ){
                        current->end_time = time(NULL);
                        current->execution_time = current->end_time - current->start_time;
                        current->task_pid = 0;
                        if( WEXITSTATUS(status) == 0 ) {
                            printf("Task %d completed successfully; total time=%ld\n", current->task_id, current->execution_time);
                            current->status = COMPLETED;
                        }
                        else{
                            printf("Task %d failed; total time=%ld\n", current->task_id, current->execution_time);
                            current->status = FAILED;
                        }
                    }
                    else{
                        //Running?
                    }
                }
            }
        }
        current = current->next;
    }
}


/// @brief sleeps a certain amount of milliseconds
void sleepms( int ms ){
    struct timespec req, rem;
    req.tv_sec = 0;                  // Seconds
    req.tv_nsec = 1000 * 1000 * ms; // 100 milliseconds to nanoseconds
    nanosleep(&req, &rem);
}


/// @brief Set file descriptor to non-blocking mode
int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    flags |= O_NONBLOCK;

    int result = fcntl(fd, F_SETFL, flags);
    return result;
}


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
    Task *current_scheduled = task_queue;  // Ensure this points to the archive_queue
    if (current == NULL && current_scheduled == NULL) {
        write(fd, "No tasks in the queue.\n", 22);
        close(fd);
        return;
    }
    //
    // Running or runned tasks
    while (current != NULL) {
        dprintf(fd, "Task %d: %s\n", current->task_id,
               (current->status == SCHEDULED) ? "Scheduled" :
               (current->status == EXECUTING) ? "Executing" :
               (current->status == COMPLETED) ? "Completed" : "Failed");
        current = current->next;
    }
    //
    // Scheduled tasks
    current = current_scheduled;  // Ensure this points to the task_queue
    while (current != NULL) {
        dprintf(fd, "Task %d: %s\n", current->task_id,
               (current->status == SCHEDULED) ? "Scheduled" :
               (current->status == EXECUTING) ? "Executing" :
               (current->status == COMPLETED) ? "Completed" : "Failed");
        current = current->next;
    }
    
    close(fd);  // Close the FIFO after all writes
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



/// @brief Add a task to the queue
/// @param command 
/// @return 0 if successful, -1 otherwise
int schedule_task(Command *command){
    int result = 0;

    if (command->type == EXECUTE)
    {
        Command *new_command = malloc(sizeof(Command));
        if (new_command == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            result = EXIT_FAILURE;
        }
        memcpy(new_command, command, sizeof(Command));

        // Create and enqueue a new task based on the command
        Task *new_task = malloc(sizeof(Task));
        if (new_task == NULL) {
            fprintf(stderr, "Memory allocation failed for new task\n");
            free(new_command);  // Free previously allocated memory
            result = EXIT_FAILURE;
        }
        new_task->command = new_command;
        new_task->task_id = ++task_counter;
        new_task->status = SCHEDULED;
        new_task->next = NULL;
        //
        enqueue_task( &task_queue, new_task );
    }
    return result;
}


/// @brief returns the number of running tasks
/// @return 
int get_running_tasks(){
    int running_tasks = 0;
    Task *current = archive_queue;
    while (current != NULL) {
        if (current->status == EXECUTING) {
            running_tasks++;
        }
        current = current->next;
    }
    return running_tasks;
}



/// @brief Process next task in queue
int handle_process_queue(){
    int result = 0;

    // Check if the number of running tasks is less than the maximum allowed
    if(get_running_tasks() >= max_parallel_tasks){
        return result;
    }
    // Get the next task from the queue
    Task *current = dequeue_task( &task_queue );
    if( current != NULL ) {
        enqueue_task( &archive_queue, current);
        process_command(current, output_folder);

        result = 1;
    }
    return result;
}



/// @brief Wait for commands main loop
void setup_fifo(const char *fifo_path)
{
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

    // set non-blocking mode
    set_non_blocking(fifo_fd);

    while(1){
        char buffer[MAX_SZ];
        // Read from the FIFO
        int nbytes = read(fifo_fd, buffer, sizeof(buffer) - 1);
        if (nbytes > 0) {
            buffer[nbytes] = '\0';  // Null-terminate the string
            // printf("Read %d bytes: %s", nbytes, buffer);
            // Parse the command and add to queue
            Command command;
            parse_command(buffer, &command);
            switch (command.type)
            {
                case EXECUTE:
                    schedule_task( &command );
                    break;
                
                case STATUS:
                    send_status_over_fifo();
                    break;
                
                default:
                    break;
            }

        } else if (nbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Debug
            printf("No data available, try again later.\n");
        } else if(nbytes == -1 ) {
            // Error: Pipe broken
            perror("read: pipe broken\n");
            break;
        }
        // update running tasks status
        update_task_status();
        // process next task in queue
        if( handle_process_queue() == 0 ){
            // No more tasks to process / or wainting for tasks to finish
            // Sleep for some time
            sleepms(250);
        }
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
    printf("Server waiting for commands on: %s\n", fifo_path);

    // Wait for commands
    setup_fifo(fifo_path);

    return 0;
}