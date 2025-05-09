#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "pipeline.h"


int pomelo_pipeline_init(
    pomelo_pipeline_t * pipeline,
    pomelo_pipeline_options_t * options
) {
    assert(options != NULL);
    memset(pipeline, 0, sizeof(pomelo_pipeline_t));

    pipeline->tasks = options->tasks;
    pipeline->task_count = options->task_count;
    pipeline->callback_data = options->callback_data;
    pipeline->sequencer = options->sequencer;

    // Initialize the sequencer task
    pomelo_sequencer_task_init(
        &pipeline->sequencer_task,
        (pomelo_sequencer_callback) pomelo_pipeline_execute_current_task,
        pipeline
    );

    return 0;
}


void pomelo_pipeline_cleanup(pomelo_pipeline_t * pipeline) {
    assert(pipeline != NULL);
    pipeline->tasks = NULL;
    pipeline->task_count = 0;
}


void pomelo_pipeline_start(pomelo_pipeline_t * pipeline) {
    assert(pipeline != NULL);

    // Reset the task index
    pipeline->task_index = 0;
    pipeline->flags = 0; // Reset the flags

    if (pipeline->task_count == 0) return; // No tasks to run

    // Call the first task
    pomelo_pipeline_execute(pipeline);
}


void pomelo_pipeline_next(pomelo_pipeline_t * pipeline) {
    assert(pipeline != NULL);
    if (pipeline->task_index == (pipeline->task_count - 1)) {
        return; // Already finished
    }

    if (pipeline->flags & POMELO_PIPELINE_FLAG_BUSY) {
        // In the case of busy, just set the flag
        pipeline->flags |= POMELO_PIPELINE_FLAG_NEXT;
        return;
    }

    // Execute the next task
    pipeline->task_index++;
    pomelo_pipeline_execute(pipeline);
}


void pomelo_pipeline_finish(pomelo_pipeline_t * pipeline) {
    assert(pipeline != NULL);
    if (pipeline->task_index == (pipeline->task_count - 1)) {
        return; // Already finished
    }

    if (pipeline->flags & POMELO_PIPELINE_FLAG_BUSY) {
        // In the case of busy, just set the flag
        pipeline->flags |= POMELO_PIPELINE_FLAG_FINISH;
        return;
    }

    pipeline->task_index = pipeline->task_count - 1;
    pomelo_pipeline_execute(pipeline);
}


void pomelo_pipeline_execute(pomelo_pipeline_t * pipeline) {
    assert(pipeline != NULL);

    pipeline->flags |= POMELO_PIPELINE_FLAG_BUSY;
    pomelo_sequencer_t * sequencer = pipeline->sequencer;

    while (true) {
        // Run the task
        if (sequencer) {
            // Submit the task to the sequencer
            pomelo_sequencer_submit(sequencer, &pipeline->sequencer_task);
        } else {
            // Execute the task directly
            pomelo_pipeline_execute_current_task(pipeline);
        }

        // Check the finish flag
        if (pipeline->flags & POMELO_PIPELINE_FLAG_FINISH) {
            pipeline->flags &= ~POMELO_PIPELINE_FLAG_FINISH;
            // Run the last task
            if (pipeline->task_index != (pipeline->task_count - 1)) {
                pipeline->task_index = pipeline->task_count - 1;
                continue; // Continue the loop
            }
        }

        // Check the next flag
        if (pipeline->flags & POMELO_PIPELINE_FLAG_NEXT) {
            pipeline->flags &= ~POMELO_PIPELINE_FLAG_NEXT;
            // Run the next task
            if (pipeline->task_index != (pipeline->task_count - 1)) {
                pipeline->task_index++;
                continue; // Continue the loop
            }
        }

        break; // Break the loop
    }

    pipeline->flags &= ~POMELO_PIPELINE_FLAG_BUSY;
}


void pomelo_pipeline_execute_current_task(pomelo_pipeline_t * pipeline) {
    assert(pipeline != NULL);
    assert(pipeline->tasks != NULL);
    assert(pipeline->task_index < pipeline->task_count);
    assert(pipeline->tasks[pipeline->task_index] != NULL);

    pipeline->tasks[pipeline->task_index](pipeline->callback_data);
}
