#ifndef POMELO_BASE_SEQUENCER_H
#define POMELO_BASE_SEQUENCER_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/// @brief The manager of sequence tasks
///
/// The sequence manager ensures sequential task execution by enforcing that
/// each task must complete before the next one can begin. When a task is
/// submitted while another is running, it will be queued and only executed
/// after the current task fully returns. This guarantees ordered,
/// non-overlapping task execution.


/// @brief The manager of sequence tasks
typedef struct pomelo_sequencer_s pomelo_sequencer_t;

/// @brief The entry of sequence task
typedef void (*pomelo_sequencer_callback)(void * data);

/// @brief The task of sequence
typedef struct pomelo_sequencer_task_s pomelo_sequencer_task_t;


struct pomelo_sequencer_s {
    /// @brief The head of pending list
    pomelo_sequencer_task_t * head;

    /// @brief The tail of pending list
    pomelo_sequencer_task_t * tail;

    /// @brief Whether the manager is busy
    bool busy;
};


struct pomelo_sequencer_task_s {
    /// @brief The callback of task
    pomelo_sequencer_callback callback;

    /// @brief The callback data of task
    void * data;

    /// @brief The next task in the pending list
    pomelo_sequencer_task_t * next;

    /// @brief The previous task in the pending list
    pomelo_sequencer_task_t * prev;

    /// @brief Whether the task is pending
    bool pending;
};


/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Initialize a sequencer
/// @brief This function will always return 0
int pomelo_sequencer_init(pomelo_sequencer_t * sequencer);


/// @brief Submit a task.
/// @brief This function will always return 0
int pomelo_sequencer_submit(
    pomelo_sequencer_t * sequencer,
    pomelo_sequencer_task_t * task
);


/// @brief Initialize a sequence task
/// @brief This function will always return 0
int pomelo_sequencer_task_init(
    pomelo_sequencer_task_t * task,
    pomelo_sequencer_callback callback,
    void * data
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_BASE_SEQUENCER_H
