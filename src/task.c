#include <netinet/in.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "task.h"

int process_command( Task *task, char *output_folder ) {
    int result = 0;

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
        int exec_result = execvp(args[0], args+1);
        if(exec_result == -1) {
            fprintf(stderr, "Failed to execute command. errno=%d\n", errno);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else if( pid > 0 ) {
        // parent process
        result = pid;
        task->task_pid = pid;
        task->status = EXECUTING;
    }
    else {
        // Fork failed
        fprintf(stderr, "Fork failed\n");
        //exit(EXIT_FAILURE);
    }

    return result;
}
