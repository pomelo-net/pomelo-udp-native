#ifndef POMELO_PLATFORM_UV_EXECUTOR_SRC_H
#define POMELO_PLATFORM_UV_EXECUTOR_SRC_H
#include "utils/list.h"
#include "utils/atomic.h"
#include "utils/pool.h"
#include "platform-uv.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Busy flag of executor
#define POMELO_EXECUTOR_FLAG_BUSY       (1 << 0)

/// @brief Shutdown flag of executor
#define POMELO_EXECUTOR_FLAG_SHUTDOWN   (1 << 1)


/// @brief Threadsafe task
typedef struct pomelo_platform_task_threadsafe_s
    pomelo_platform_task_threadsafe_t;


struct pomelo_platform_threadsafe_controller_s {
    /// @brief Platform
    pomelo_platform_t * platform;

    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief UV loop
    uv_loop_t * uv_loop;

    /// @brief Pool of tasks
    pomelo_pool_t * task_pool;

    /// @brief Pool of executors
    pomelo_pool_t * executor_pool;

    /// @brief Running flag of controller
    pomelo_atomic_int64_t running;

    /// @brief Counter of tasks
    pomelo_atomic_uint64_t task_counter;

    /// @brief List of executors
    pomelo_list_t * executors;
};


struct pomelo_threadsafe_executor_s {
    /// @brief Controller
    pomelo_platform_threadsafe_controller_t * controller;

    /// @brief Running flag of executor
    pomelo_atomic_int64_t running;

    /// @brief Front tasks list for queuing
    pomelo_list_t * tasks_front;

    /// @brief Back tasks list for executing
    pomelo_list_t * tasks_back;

    /// @brief Mutex of controller
    uv_mutex_t mutex;

    /// @brief UV async
    uv_async_t uv_async;

    /// @brief Flags of executor
    uint32_t flags;

    /// @brief Entry of this executor
    pomelo_list_entry_t * entry;
};


struct pomelo_platform_task_threadsafe_s {
    /// @brief Controller
    pomelo_platform_threadsafe_controller_t * controller;

    /// @brief Entry of this task
    pomelo_platform_task_entry entry;

    /// @brief Data of this task
    void * data;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Create threadsafe controller
pomelo_platform_threadsafe_controller_t *
pomelo_platform_threadsafe_controller_create(
    pomelo_platform_t * platform,
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy threadsafe controller
void pomelo_platform_threadsafe_controller_destroy(
    pomelo_platform_threadsafe_controller_t * controller
);


/// @brief Startup the threadsafe controller
void pomelo_platform_threadsafe_controller_startup(
    pomelo_platform_threadsafe_controller_t * controller
);


/// @brief Shutdown the threadsafe controller
void pomelo_platform_threadsafe_controller_shutdown(
    pomelo_platform_threadsafe_controller_t * controller
);


/// @brief Callback when the threadsafe controller is completely shutdown
void pomelo_platform_threadsafe_controller_on_shutdown(
    pomelo_platform_threadsafe_controller_t * controller
);


/// @brief Get statistic information
void pomelo_platform_threadsafe_controller_statistic(
    pomelo_platform_threadsafe_controller_t * controller,
    pomelo_statistic_platform_uv_t * statistic
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Release threadsafe task
void pomelo_platform_task_threadsafe_release(
    pomelo_platform_task_threadsafe_t * task
);


/// @brief Async callback
void pomelo_platform_task_threadsafe_async_callback(uv_async_t * async);


/// @brief Initialize the threadsafe executor
int pomelo_threadsafe_executor_on_alloc(
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_threadsafe_controller_t * controller
);


/// @brief Finalize the threadsafe executor
void pomelo_threadsafe_executor_on_free(
    pomelo_threadsafe_executor_t * executor
);


/// @brief Startup the threadsafe executor
int pomelo_threadsafe_executor_startup(
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_threadsafe_controller_t * controller
);


/// @brief Shutdown the threadsafe executor
int pomelo_threadsafe_executor_shutdown(
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_threadsafe_controller_t * controller
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_EXECUTOR_SRC_H
