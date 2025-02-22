#ifndef POMELO_PLATFORM_UV_TASK_MAIN_SRC_H
#define POMELO_PLATFORM_UV_TASK_MAIN_SRC_H
#include "uv.h"
#include "platform/platform.h"
#include "utils/list.h"
#include "utils/atomic.h"
#include "utils/pool.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Main task controller
typedef struct pomelo_platform_task_main_controller_s
    pomelo_platform_task_main_controller_t;


/// @brief Main task
typedef struct pomelo_platform_task_main_s pomelo_platform_task_main_t;


struct pomelo_platform_task_main_controller_s {
    /// @brief Platform of controller
    pomelo_allocator_t * allocator;

    /// @brief UV loop
    uv_loop_t * uv_loop;

    /// @brief Running flag of controller
    pomelo_atomic_int64_t running;

    /// @brief UV async
    uv_async_t uv_async;

    /// @brief Pool of task
    pomelo_pool_t * task_pool;

    /// @brief Front tasks list for queuing
    pomelo_list_t * tasks_front;

    /// @brief Back tasks list for executing
    pomelo_list_t * tasks_back;

    /// @brief Mutex of controller
    uv_mutex_t mutex;
};


struct pomelo_platform_task_main_s {
    /// @brief Controller
    pomelo_platform_task_main_controller_t * controller;

    /// @brief Callback of this task
    pomelo_platform_task_cb callback;

    /// @brief Callback data
    void * callback_data;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Create task main controller
pomelo_platform_task_main_controller_t *
pomelo_platform_task_main_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy task main controller
void pomelo_platform_task_main_controller_destroy(
    pomelo_platform_task_main_controller_t * controller
);


/// @brief Startup the main controller
void pomelo_platform_task_main_controller_startup(
    pomelo_platform_task_main_controller_t * controller
);


/// @brief Shutdown the main controller
void pomelo_platform_task_main_controller_shutdown(
    pomelo_platform_task_main_controller_t * controller
);


/// @brief Get statistic information
void pomelo_platform_task_main_controller_statistic(
    pomelo_platform_task_main_controller_t * controller,
    pomelo_statistic_platform_t * statistic
);


/// @brief Submit a task
int pomelo_platform_task_main_controller_submit(
    pomelo_platform_task_main_controller_t * controller,
    pomelo_platform_task_cb callback,
    void * callback_data
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Release main task
void pomelo_platform_task_main_release(pomelo_platform_task_main_t * task);


/// @brief Async callback
void pomelo_platform_task_main_async_callback(uv_async_t * async);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_TASK_MAIN_SRC_H
