#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "queue.h"
#include "task.h"

extern Task *task_queue;
extern Task *archive_queue;

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

/*
void enqueue_task(Task *new_task) {
    // Enqueue in processing queue
    if (task_queue == NULL) {
        task_queue = new_task;
    } else {
        Task *current = task_queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_task;
    }
}
*/
    /*
    // Create a separate task for the archive queue to maintain a history
    Task *archive_task = malloc(sizeof(Task));
    if (archive_task == NULL) {
        fprintf(stderr, "Memory allocation failed for archive task\n");
        exit(EXIT_FAILURE);
    }
    *archive_task = *new_task;  // Shallow copy of the task structure

    archive_task->command = malloc(sizeof(Command));
    if (archive_task->command == NULL) {
        fprintf(stderr, "Memory allocation failed for command in archive task\n");
        exit(EXIT_FAILURE);
    }
    *archive_task->command = *new_task->command;  // Deep copy of the command

    // Enqueue in archive queue
    if (archive_queue == NULL) {
        archive_queue = archive_task;
    } else {
        Task *current_archive = archive_queue;
        while (current_archive->next != NULL) {
            current_archive = current_archive->next;
        }
        current_archive->next = archive_task;
    }
    */

    //printf("Task ID %d enqueued: %s\n", new_task->task_id, new_task->command->args);
//}

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
