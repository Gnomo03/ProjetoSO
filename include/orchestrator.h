#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <netinet/in.h>

#define MAX_SZ 1024

typedef enum
{
    EXECUTE,
    STATUS
} CommandType;

typedef struct
{
    CommandType type;
    int time;
    char flag[MAX_SZ];
    char args[MAX_SZ];
} Command;

typedef struct Task
{
    Command *command;
    struct Task *next;
} Task;

void execute_command(const char *cmd);
void setup_fifo(const char *fifo_path);
void parse_command(const char *input, Command *command);
void process_command(Command *command);
Task *dequeue_task();
void enqueue_task(Command *command);

#endif
