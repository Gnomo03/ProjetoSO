#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <netinet/in.h>

#define MAX_SZ 1024
#define RESPONSE_FIFO_PATH "/tmp/response_fifo"
#define FIFO_PATH "/tmp/server_fifo" 

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
    int task_id;
    Command *command;
    struct Task *next;
    TaskStatus status;
    time_t execution_time;
    time_t start_time; 
    time_t end_time; 
} Task;

void execute_command(const char *cmd);
void setup_fifo(const char *fifo_path);
void parse_command(const char *input, Command *command);
void process_command(Command *command);
Task *dequeue_task();
void enqueue_task(Command *command);
void setup_response_fifo();
void send_status_over_fifo();
Task *add_task_to_queue(int task_id);
Task *get_next_unprocessed_task();

#endif
