
#ifndef TASK_H
    #define TASK_H

#include <time.h>

#define MAX_SZ 1024


typedef enum {
    EXECUTE,
    STATUS
} CommandType;

typedef enum {
    SCHEDULED,
    EXECUTING,
    COMPLETED,
    FAILED
} TaskStatus;

typedef struct {
    CommandType type;
    int time;
    char flag[MAX_SZ];
    char args[MAX_SZ];
} Command;

typedef struct Task {
    int task_pid;
    int task_id;
    Command *command;
    struct Task *next;
    TaskStatus status;
    time_t execution_time;
    time_t start_time; 
    time_t end_time; 
} Task;

int process_command( Task *task, char *output_folder );

#endif