
#ifndef QUEUE_H
#define QUEUE_H

#include "task.h"

void update_archive_pid( Task **queue, int task_id, int pid);
void update_archive_task_pid_status( Task **queue, int task_id, int pid, TaskStatus new_status);
void enqueue_task( Task **queue, Task *new_task);
Task *dequeue_task( Task **queue );
void enqueue_archive_task( Task **queue, Task *new_task);

#endif
