#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <netinet/in.h>

#define MAX_SZ 1024

typedef enum
{
    EXECUTE,
    STATUS
} CommandType;

typedef enum {
    WAITING,
    RUNNING,
    COMPLETED
} TaskState;

typedef struct
{
    CommandType type;
    int time;
    char flag[MAX_SZ];
    char args[MAX_SZ];
} Command;

typedef struct Task
{
    int task_id; 
    Command *command;
    struct Task *next;
    TaskState state;
    time_t start_time;
    time_t end_time;

} Task;

void execute_command(const char *cmd);
void setup_fifo(const char *fifo_path);
void parse_command(const char *input, Command *command);
void process_command(Command *command);
Task *dequeue_task();
void enqueue_task(Command *command);
char *get_task_status();

#endif
