#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "queue.h"
#include "task.h"

extern Task *task_queue;
extern Task *archive_queue;

/*
void update_pid(Task **queue, int task_id, int pid) {
    Task *current = *queue;
    while (current != NULL) {
        if (current->task_id == task_id) {
            current->task_pid = pid;
            break;
        }
        current = current->next;
    }
}

void update_pid_status(Task **queue, int task_id, int pid, TaskStatus new_status) {
    Task *current = *queue;
    while (current != NULL) {
        if (current->task_id == task_id) {
            if( pid > 0){
                current->task_pid = pid;
            }
            current->status = new_status;
            break;
        }
        current = current->next;
    }
}
*/

void enqueue_task( Task **queue,  Task *new_task) {
    // Enqueue in processing queue
    if (*queue == NULL) {
        *queue = new_task;
    } else {
        Task *current = *queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_task;
        new_task->next = NULL;
    }
}


Task *dequeue_task( Task **queue )
{
    if (*queue == NULL)
    {
        return NULL;
    }
    else
    {
        Task *temp = *queue;
        *queue = task_queue->next;
        return temp;
    }
}
