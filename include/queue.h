
#ifndef QUEUE_H
#define QUEUE_H

#include "../include/task.h"

Task *task_queue = NULL;
Task *archive_queue = NULL;

void update_archive_pid(int task_id, int pid);
void update_archive_task_pid_status(int task_id, int pid, TaskStatus new_status);
void enqueue_task(Task *new_task);
Task *dequeue_task();

#endif
