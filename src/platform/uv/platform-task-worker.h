#ifndef POMELO_PLATFORM_UV_TASK_WORKER_SRC_H
#define POMELO_PLATFORM_UV_TASK_WORKER_SRC_H
#include "uv.h"
#include "platform/platform.h"
#include "utils/pool.h"
#include "utils/list.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Worker task controller
typedef struct pomelo_platform_task_worker_controller_s
    pomelo_platform_task_worker_controller_t;


/// @brief Worker task
typedef struct pomelo_platform_task_worker_s pomelo_platform_task_worker_t;


struct pomelo_platform_task_worker_controller_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief UV Loop
    uv_loop_t * uv_loop;

    /// @brief Running flag
    bool running;

    /// @brief Pool of worker tasks
    pomelo_pool_t * task_pool;

    /// @brief Processing tasks
    pomelo_list_t * tasks;
};


struct pomelo_platform_task_worker_s {
    /// @brief Controller
    pomelo_platform_task_worker_controller_t * controller;

    /// @brief The entry point of work
    pomelo_platform_task_cb entry;

    /// @brief The done point of work
    pomelo_platform_task_done_cb done;

    /// @brief Callback data
    void * callback_data;

    /// @brief Canceled flag
    bool canceled;

    /// @brief UV work
    uv_work_t uv_work;

    /// @brief Group of this task
    pomelo_platform_task_group_t * group;

    /// @brief Node of this task in the list of controller
    pomelo_list_node_t * global_node;

    /// @brief Node of this task in the list of group
    pomelo_list_node_t * group_node;

    /// @brief Cancel callback
    pomelo_platform_task_cb cancel_callback;

    /// @brief Cancel callback data
    void * cancel_callback_data;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Create task worker controller
pomelo_platform_task_worker_controller_t *
pomelo_platform_task_worker_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy task worker controller
void pomelo_platform_task_worker_controller_destroy(
    pomelo_platform_task_worker_controller_t * controller
);


/// @brief Startup the controller
void pomelo_platform_task_worker_controller_startup(
    pomelo_platform_task_worker_controller_t * controller
);


/// @brief Shutdown the controller
void pomelo_platform_task_worker_controller_shutdown(
    pomelo_platform_task_worker_controller_t * controller
);


/// @brief Get statistic information
void pomelo_platform_task_worker_controller_statistic(
    pomelo_platform_task_worker_controller_t * controller,
    pomelo_statistic_platform_t * statistic
);


/// @brief Submit worker task
int pomelo_platform_task_worker_controller_submit(
    pomelo_platform_task_worker_controller_t * controller,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb entry,
    pomelo_platform_task_done_cb done,
    void * callback_data
);


/// @brief Cancel worker task.
/// The callback will not be called anymore.
/// After canceled, provided callback will be called. If task has already been
/// canceled, cancel callback will not be called.
void pomelo_platform_task_worker_cancel(
    pomelo_platform_task_worker_t * task,
    pomelo_platform_task_cb callback,
    void * callback_data
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief The entry point of worker task
void pomelo_platform_task_worker_entry(uv_work_t * uv_work);


/// @brief The done point of worker task
void pomelo_platform_task_worker_done(uv_work_t * uv_work, int status);


/// @brief Release task worker
void pomelo_platform_task_worker_release(pomelo_platform_task_worker_t * task);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_TASK_WORKER_SRC_H
