#ifndef POMELO_BASE_PIPELINE_H
#define POMELO_BASE_PIPELINE_H
#include <stddef.h>
#include <stdint.h>
#include "sequencer.h"
#ifdef __cplusplus
extern "C" {
#endif

/// @brief Busy flag
#define POMELO_PIPELINE_FLAG_BUSY   (1 << 0)

/// @brief Next flag
#define POMELO_PIPELINE_FLAG_NEXT   (1 << 1)

/// @brief Finish flag
#define POMELO_PIPELINE_FLAG_FINISH (1 << 2)


/// @brief Serial of tasks
typedef struct pomelo_pipeline_s pomelo_pipeline_t;

/// @brief Pipeline options
typedef struct pomelo_pipeline_options_s pomelo_pipeline_options_t;

/// @brief Task function
typedef void (*pomelo_pipeline_entry_fn)(void * data);


struct pomelo_pipeline_s {
    /// @brief Tasks
    pomelo_pipeline_entry_fn * tasks;

    /// @brief Task count
    size_t task_count;

    /// @brief Current task index
    size_t task_index;

    /// @brief Flags
    uint32_t flags;

    /// @brief Callback data
    void * callback_data;

    /// @brief The sequencer. If not NULL, the pipeline tasks will be submitted
    /// to the sequencer.
    pomelo_sequencer_t * sequencer;

    /// @brief The sequencer task
    pomelo_sequencer_task_t sequencer_task;
};


struct pomelo_pipeline_options_s {
    /// @brief Tasks
    pomelo_pipeline_entry_fn * tasks;

    /// @brief Task count
    size_t task_count;

    /// @brief Callback data
    void * callback_data;

    /// @brief The sequencer. If not NULL, the pipeline tasks will be submitted
    /// to the sequencer.
    pomelo_sequencer_t * sequencer;
};

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Create a new pipeline
int pomelo_pipeline_init(
    pomelo_pipeline_t * pipeline,
    pomelo_pipeline_options_t * options
);


/// @brief Destroy a pipeline
void pomelo_pipeline_cleanup(pomelo_pipeline_t * pipeline);


/// @brief Start the pipeline. This will call the first task in the pipeline.
void pomelo_pipeline_start(pomelo_pipeline_t * pipeline);


/// @brief Run the next task in the pipeline.
/// This will call the next task in the pipeline.
void pomelo_pipeline_next(pomelo_pipeline_t * pipeline);


/// @brief Finish the pipeline. This will call the last task in the pipeline.
void pomelo_pipeline_finish(pomelo_pipeline_t * pipeline);


/* -------------------------------------------------------------------------- */
/*                              Private APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Execute the pipeline
void pomelo_pipeline_execute(pomelo_pipeline_t * pipeline);


/// @brief Execute the pipeline deferred
void pomelo_pipeline_execute_current_task(pomelo_pipeline_t * pipeline);


#ifdef __cplusplus
}
#endif
#endif // POMELO_BASE_PIPELINE_H
