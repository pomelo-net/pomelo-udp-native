#include <assert.h>
#include <string.h>
#include "sequencer.h"


int pomelo_sequencer_init(pomelo_sequencer_t * sequencer) {
    assert(sequencer != NULL);
    memset(sequencer, 0, sizeof(pomelo_sequencer_t));
    return 0;
}


int pomelo_sequencer_submit(
    pomelo_sequencer_t * sequencer,
    pomelo_sequencer_task_t * task
) {
    assert(sequencer != NULL);
    assert(task != NULL);
    assert(task->callback != NULL);  // Ensure callback is set

    if (task->pending) {
        // If the task is already pending, just move it to the tail of the list
        // Unlink the task from the list
        if (task->prev) {
            task->prev->next = task->next;
        } else {
            sequencer->head = task->next;
        }

        if (task->next) {
            task->next->prev = task->prev;
        } else {
            sequencer->tail = task->prev;
        }
    }

    // Append the task to the tail of list
    if (!sequencer->head) {
        assert(sequencer->tail == NULL);
        sequencer->head = task;
        sequencer->tail = task;
        task->next = NULL;
        task->prev = NULL;
    } else {
        assert(sequencer->tail != NULL);
        sequencer->tail->next = task;
        task->next = NULL;
        task->prev = sequencer->tail;
        sequencer->tail = task;
    }
    task->pending = true;

    if (sequencer->busy) {
        // If sequencer is busy, return immediately
        return 0;
    }

    // Set sequencer busy
    sequencer->busy = true;

    // Execute the tasks
    while (sequencer->head) {
        // Pop the head of the list
        pomelo_sequencer_task_t * current = sequencer->head;
        sequencer->head = current->next;
        if (sequencer->head) {
            sequencer->head->prev = NULL;
        } else {
            sequencer->tail = NULL;
        }

        // Execute the task
        current->pending = false;
        current->callback(current->data);
    }

    // Set sequencer idle
    sequencer->busy = false;

    // At this point, head and tail must be NULL
    assert(sequencer->head == NULL);
    assert(sequencer->tail == NULL);
    return 0;
}


int pomelo_sequencer_task_init(
    pomelo_sequencer_task_t * task,
    pomelo_sequencer_callback callback,
    void * data
) {
    assert(task != NULL);
    assert(callback != NULL);
    task->callback = callback;
    task->data = data;
    task->next = NULL;
    task->pending = false;
    return 0;
}
