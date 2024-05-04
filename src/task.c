#include <netinet/in.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "../include/task.h"

int process_command( Task *task, char *output_folder ) {
    int fd[2];    // Pipe file descriptors

    // Update status to EXECUTING
    task->start_time = time(NULL);

    pid_t pid = fork();
    if (pid == 0) {        // child process
        // Output file
        char output_path[MAX_SZ];
        FILE *output_file;

        // Construct output file path for the task logs
        snprintf(output_path, MAX_SZ, "%s/execution_log_TASK%d.txt", output_folder, task->task_id);
        output_file = fopen(output_path, "a");
        if (output_file == NULL) {
            fprintf(stderr, "Failed to open output file\n");
            exit(EXIT_FAILURE);
        }

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
    }
    else if( pid > 0 ) {
        // parent process
        task->task_pid = pid;
        
        // Wait for child process to complete
        waitpid(pid, NULL, 0);
        
        task->status = COMPLETED;
        task->end_time = time(NULL);
        task->execution_time = task->end_time - task->start_time;
    }
    } else {
        // Fork failed
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
    }
}