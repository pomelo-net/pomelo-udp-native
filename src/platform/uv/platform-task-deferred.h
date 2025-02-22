#ifndef POMELO_PLATFORM_UV_TASK_DEFERRED_SRC_H
#define POMELO_PLATFORM_UV_TASK_DEFERRED_SRC_H
#include "uv.h"
#include "platform/platform.h"
#include "utils/pool.h"
#include "utils/list.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Deferred task controller
typedef struct pomelo_platform_task_deferred_controller_s
    pomelo_platform_task_deferred_controller_t;



/// @brief Deferred task
typedef struct pomelo_platform_task_deferred_s pomelo_platform_task_deferred_t;


struct pomelo_platform_task_deferred_controller_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief UV loop
    uv_loop_t * uv_loop;

    /// @brief Running flag
    bool running;

    /// @brief Pool of tasks
    pomelo_pool_t * task_pool;

    /// @brief Processing tasks
    pomelo_list_t * tasks;
};


struct pomelo_platform_task_deferred_s {
    /// @brief Callback of task
    pomelo_platform_task_cb callback;

    /// @brief Callback data
    void * callback_data;

    /// @brief Canceled flag
    bool canceled;

    /// @brief Controller
    pomelo_platform_task_deferred_controller_t * controller;

    /// @brief Idle for this deferred task
    uv_idle_t idle;

    /// @brief Task group this task belongs to
    pomelo_platform_task_group_t * group;

    /// @brief Node of this task in deferred list of task group
    pomelo_list_node_t * group_node;

    /// @brief Node of this task in tasks list of controller
    pomelo_list_node_t * global_node;

};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Create task deferred controller
pomelo_platform_task_deferred_controller_t *
pomelo_platform_task_deferred_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy task deferred controller
void pomelo_platform_task_deferred_controller_destroy(
    pomelo_platform_task_deferred_controller_t * controller
);


/// @brief Startup the deferred controller
void pomelo_platform_task_deferred_controller_startup(
    pomelo_platform_task_deferred_controller_t * controller
);


/// @brief Shutdown the deferred controller
void pomelo_platform_task_deferred_controller_shutdown(
    pomelo_platform_task_deferred_controller_t * controller
);


/// @brief Get statistic information
void pomelo_platform_task_deferred_controller_statistic(
    pomelo_platform_task_deferred_controller_t * controller,
    pomelo_statistic_platform_t * statistic
);


/// @brief Submit a task
int pomelo_platform_task_deferred_controller_submit(
    pomelo_platform_task_deferred_controller_t * controller,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb callback,
    void * callback_data
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Deferred task callback
void pomelo_platform_task_deferred_callback(uv_idle_t * idle);


/// @brief Release deferred task
void pomelo_platform_task_deferred_release(
    pomelo_platform_task_deferred_t * task
);


/// @brief Cancel deferred task
void pomelo_platform_task_deferred_cancel(
    pomelo_platform_task_deferred_t * task
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_TASK_DEFERRED_SRC_H
