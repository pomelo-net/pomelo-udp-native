#ifndef POMELO_PLATFORM_POLL_TASK_SRC_H
#define POMELO_PLATFORM_POLL_TASK_SRC_H
#include <stdbool.h>
#include "uv.h"
#include "platform/platform.h"
#include "utils/list.h"
#include "utils/pool.h"
#include "utils/atomic.h"
#ifdef __cplusplus
extern "C" {
#endif


/*
    - Main tasks are the same as UV platform.
    - Deferred tasks and worker tasks will be processed lately at the end of
    polling process
*/


/// @brief Task controller
typedef struct pomelo_platform_task_controller_s
    pomelo_platform_task_controller_t;

/// @brief Late task, this is for deferred & worker
typedef struct pomelo_platform_task_late_s pomelo_platform_task_late_t;

/// @brief Main task
typedef struct pomelo_platform_task_main_s pomelo_platform_task_main_t;


struct pomelo_platform_task_controller_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief UV loop
    uv_loop_t * uv_loop;

    /// @brief Mutex for this controller
    uv_mutex_t uv_mutex;

    /// @brief Running flag
    pomelo_atomic_int64_t running;

    /// @brief Pool of late tasks
    pomelo_pool_t * task_late_pool;

    /// @brief Pool of main tasks
    pomelo_pool_t * task_main_pool;

    /// @brief Main task available flag
    pomelo_atomic_int64_t main_tasks_available;

    /// @brief Front list of main tasks
    pomelo_list_t * main_tasks_front;

    /// @brief Back list of main tasks
    pomelo_list_t * main_tasks_back;

    /// @brief Front list of late tasks
    pomelo_list_t * late_tasks_front;

    /// @brief Back list of late tasks
    pomelo_list_t * late_tasks_back;

    /// @brief Deferred task count
    size_t deferred_task_count;

    /// @brief Worker task count
    size_t worker_task_count;
};


struct pomelo_platform_task_group_s {
    /// @brief Tasks list
    pomelo_list_t * tasks;

    /// @brief Cancel callback
    pomelo_platform_task_cb cancel_callback;

    /// @brief Cancel callback data
    void * cancel_callback_data;
};


struct pomelo_platform_task_late_s {
    /// @brief Controller of task
    pomelo_platform_task_controller_t * controller;

    /// @brief Entry callback
    pomelo_platform_task_cb entry;

    /// @brief Done callback
    pomelo_platform_task_done_cb done;

    /// @brief Callback data
    void * callback_data;

    /// @brief Group of this task
    pomelo_platform_task_group_t * group;

    /// @brief Node of this task in group tasks list
    pomelo_list_node_t * group_node;

    /// @brief Canceled flag
    bool canceled;
};


struct pomelo_platform_task_main_s {
    /// @brief Controller of deferred task
    pomelo_platform_task_controller_t * controller;

    /// @brief Entry callback
    pomelo_platform_task_cb callback;

    /// @brief Callback data
    void * callback_data;

    /// @brief Global node of task
    pomelo_list_node_t * global_node;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Create new task controller
pomelo_platform_task_controller_t * pomelo_platform_task_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy task controller
void pomelo_platform_task_controller_destroy(
    pomelo_platform_task_controller_t * controller
);


/// @brief Startup the task controller
void pomelo_platform_task_controller_startup(
    pomelo_platform_task_controller_t * controller
);


/// @brief Shutdown the task controller
void pomelo_platform_task_controller_shutdown(
    pomelo_platform_task_controller_t * controller
);


/// @brief Get statistic information of platform
void pomelo_platform_task_controller_statistic(
    pomelo_platform_task_controller_t * controller,
    pomelo_statistic_platform_t * statistic
);


/// @brief Run controller
int pomelo_platform_task_controller_service(
    pomelo_platform_task_controller_t * controller
);


/// @brief Submit main task (Threadsafe)
int pomelo_platform_task_controller_submit_main(
    pomelo_platform_task_controller_t * controller,
    pomelo_platform_task_cb callback,
    void * callback_data
);


/// @brief Submit late task
int pomelo_platform_task_controller_submit_late(
    pomelo_platform_task_controller_t * controller,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb entry,
    pomelo_platform_task_done_cb done,
    void * callback_data
);


/// @brief Init task group
int pomelo_platform_task_group_init(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
);


/// @brief Reset task group
int pomelo_platform_task_group_reset(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
);

/// @brief Finalize task group
int pomelo_platform_task_group_finalize(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
);

/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Release main task
void pomelo_platform_task_main_release(pomelo_platform_task_main_t * task);


/// @brief Release late task
void pomelo_platform_task_late_release(pomelo_platform_task_late_t * task);


/// @brief Process main tasks
void pomelo_platform_task_controller_process_main_tasks(
    pomelo_platform_task_controller_t * controller
);


/// @brief Process late tasks
void pomelo_platform_task_controller_process_late_tasks(
    pomelo_platform_task_controller_t * controller
);


/// @brief Process group when a task has finished
void pomelo_platform_task_group_finish_task(
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_late_t * task
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_POLL_TASK_SRC_H
