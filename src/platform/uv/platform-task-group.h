#ifndef POMELO_PLATFORM_UV_TASK_GROUP_SRC_H
#define POMELO_PLATFORM_UV_TASK_GROUP_SRC_H
#include "platform/platform.h"
#include "utils/list.h"
#include "utils/pool.h"
#ifdef __cplusplus
extern "C" {
#endif


// Implementation of task group
struct pomelo_platform_task_group_s {
    /// @brief All processing deferred tasks
    pomelo_list_t * deferred_tasks;

    /// @brief All processing worker tasks
    pomelo_list_t * worker_tasks;

    /// @brief Cancel callback
    pomelo_platform_task_cb cancel_callback;

    /// @brief Cancel callback data
    void * cancel_callback_data;
};


/// @brief Alloc callback for task group
int pomelo_platform_task_group_init(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
);


/// @brief Acquire callback for task group
int pomelo_platform_task_group_reset(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
);


/// @brief Finalize callback for task group
int pomelo_platform_task_group_finalize(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
);


/// @brief Handle worker task has been canceled
void pomelo_platform_task_group_on_worker_task_canceled(
    pomelo_platform_task_group_t * group
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_TASK_GROUP_SRC_H
