#include <netinet/in.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>

#include "task.h"

//#define LONG_TASK


FILE * open_output_file( Task *task, char *output_folder ) {
    char output_path[MAX_SZ];
    snprintf(output_path, MAX_SZ, "%s/execution_log_TASK%d.txt", output_folder, task->task_id);
    FILE *output_file = fopen(output_path, "a");
    if (output_file == NULL) {
        fprintf(stderr, "Failed to open output file\n");
        return NULL;
    }
    return output_file;
}

void logmsg( const char *format, ...){
    FILE *log_file = fopen("log.txt", "a");
    if (log_file != NULL) {
        va_list args;
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fclose(log_file);
    }
}

void run_cmd_line( char *cmd_line, FILE *output_file) {
    // Parse command arguments
    //logmsg("run_cmd_line: cmd_line=%s\n", cmd_line);

    char *args[MAX_SZ];
    int argc = 0;
    char *token = strtok(cmd_line, " ");
    while (token != NULL) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;  // Null-terminate the args array

    //logmsg("run_cmd_line: execvp %s\t%s\t%s\tcount=%d\n", args[0], args[1], args[2], argc);
    execvp(args[0], args);
    perror("execvp");
    //fprintf( output_file, "execvp: (%d) %s\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
}


void run_cmd_list( char *cmds[], int i, FILE *output_file, int in_fd ) {
    int fd[2];

    // If this is the last command, execute it
    if (cmds[i + 1] == NULL) {
        if (in_fd != 0) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        run_cmd_line(cmds[i], output_file);
    }

    pipe(fd);
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == 0) {
        // child process
        close(fd[0]);  // Close unused read end
        if (in_fd != 0) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        run_cmd_line(cmds[i], output_file);
    }
    else if (pid > 0) {
        // parent process
        close(fd[1]);                                   // Close unused write end
        run_cmd_list(cmds, i + 1, output_file, fd[0]);  // Recurse with the next command and the read end of the pipe as the input
        close(fd[0]);                                   // Close the read end of the pipe after it's no longer needed
        waitpid(pid, NULL, 0);                          // Wait for the child process to finish
    }
    else {
        // Fork failed
        perror("Fork failed\n");
        exit(EXIT_FAILURE);
    }
}


int process_task( Task *task, char *output_folder ) {
    int result = 0;

    // Update status to EXECUTING
    gettimeofday(&(task->start_time), NULL);

    pid_t pid = fork();
    if (pid == 0) {         // child process
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
        //
        printf("Executing command: %s\n", task->command->args);

        //TODO: Retirar!!!
        // Random sleep to simulate command execution
        #ifdef LONG_TASK
        int random_sleep = rand() % 10;
        sleep(random_sleep);
        #endif
        //        
        char *cmds[MAX_SZ];
        // Parse for pipes
        if( strcasecmp(task->command->flag, "-p") ==0 ){
            //logmsg("PIPES --------------\n");
            int pipe_count = 0;
            //logmsg("Parsing: %s\n", task->command->args);
            char *token = strtok(task->command->args, "|");
            while (token != NULL && pipe_count < MAX_SZ ) {
                //logmsg("Token: %s\n", token);
                cmds[pipe_count++] = token;
                token = strtok(NULL, "|");
            }
            if( pipe_count == 0){
                cmds[pipe_count++] = task->command->args;
            }
            cmds[pipe_count] = NULL;  // Null-terminate the args array
        }
        else if( strcasecmp( task->command->flag, "-u") == 0 ) {
            cmds[0] = task->command->args;
            cmds[1] = NULL;
        }
        else{
            cmds[0] = task->command->args;
            cmds[1] = NULL;
        }
        //logmsg("PIPES END: %d\n", pipe_count);
        run_cmd_list(cmds, 0, output_file, 0);
        //
        // Only returs on error
        gettimeofday(&(task->end_time), NULL);
        task->execution_time_us = task->end_time.tv_usec - task->start_time.tv_usec;
        fprintf(output_file, "Failed to execute command. errno=%d\n", errno);
        fprintf(output_file, "Duration=%ld\n", task->execution_time_us);
        fclose(output_file);
        exit(EXIT_FAILURE);
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
        result = -1;
    }

    return result;
}


