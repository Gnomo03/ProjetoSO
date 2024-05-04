#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <netinet/in.h>
#include "task.h"

#define RESPONSE_FIFO_PATH "/tmp/response_fifo"
#define FIFO_PATH "/tmp/server_fifo" 


void execute_command(const char *cmd);
void setup_fifo(const char *fifo_path);
void parse_command(const char *input, Command *command);
void process_command(Task *task);
Task *dequeue_task();
void enqueue_task(Task *new_task);
void setup_response_fifo();
void send_status_over_fifo();
Task *add_task_to_queue(int task_id);
Task *find_task_by_id(int task_id);
Task *get_next_unprocessed_task();
void update_archive_task_status(int task_id, TaskStatus new_status);
void check_child_status( Task *task );

#endif
