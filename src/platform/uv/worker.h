#ifndef POMELO_PLATFORM_UV_TASK_WORKER_SRC_H
#define POMELO_PLATFORM_UV_TASK_WORKER_SRC_H
#include "utils/pool.h"
#include "utils/list.h"
#include "platform-uv.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Worker task
typedef struct pomelo_platform_task_worker_s pomelo_platform_task_worker_t;


struct pomelo_platform_worker_controller_s {
    /// @brief Platform
    pomelo_platform_uv_t * platform;

    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief UV Loop
    uv_loop_t * uv_loop;

    /// @brief Pool of worker tasks
    pomelo_pool_t * task_pool;

    /// @brief Processing tasks
    pomelo_list_t * tasks;

    /// @brief Running flag
    bool running;
};


struct pomelo_platform_task_worker_s {
    /// @brief Controller
    pomelo_platform_worker_controller_t * controller;

    /// @brief The entry point of work
    pomelo_platform_task_entry entry;

    /// @brief The done point of work
    pomelo_platform_task_complete complete;

    /// @brief Callback data
    void * data;

    /// @brief Canceled flag
    bool canceled;

    /// @brief UV work
    uv_work_t uv_work;

    /// @brief Entry of this task in the list of controller
    pomelo_list_entry_t * list_entry;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Create task worker controller
pomelo_platform_worker_controller_t * pomelo_platform_worker_controller_create(
    pomelo_platform_uv_t * platform,
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy task worker controller
void pomelo_platform_worker_controller_destroy(
    pomelo_platform_worker_controller_t * controller
);


/// @brief Startup the controller
void pomelo_platform_worker_controller_startup(
    pomelo_platform_worker_controller_t * controller
);


/// @brief Shutdown the controller
void pomelo_platform_worker_controller_shutdown(
    pomelo_platform_worker_controller_t * controller
);


/// @brief Callback when the controller is completely shutdown
void pomelo_platform_worker_controller_on_shutdown(
    pomelo_platform_worker_controller_t * controller
);


/// @brief Get statistic information
void pomelo_platform_worker_controller_statistic(
    pomelo_platform_worker_controller_t * controller,
    pomelo_statistic_platform_uv_t * statistic
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief The entry point of worker task
void pomelo_platform_worker_entry(uv_work_t * uv_work);


/// @brief The done point of worker task
void pomelo_platform_worker_done(uv_work_t * uv_work, int status);


/// @brief Release task worker
void pomelo_platform_worker_release(pomelo_platform_task_worker_t * task);


/// @brief Cancel worker task
void pomelo_platform_cancel_worker_task_ex(
    pomelo_platform_task_worker_t * task
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_TASK_WORKER_SRC_H
