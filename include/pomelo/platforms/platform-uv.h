#ifndef POMELO_PLATFORM_UV_H
#define POMELO_PLATFORM_UV_H
#include "uv.h"
#include "pomelo/allocator.h"
#include "pomelo/platform.h"

#ifdef __cplusplus
extern "C" {
#endif



/// @brief The options for platform
typedef struct pomelo_platform_uv_options_s pomelo_platform_uv_options_t;


/// @brief The statistic of platform
typedef struct pomelo_statistic_platform_uv_s pomelo_statistic_platform_uv_t;


struct pomelo_platform_uv_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The UV loop
    uv_loop_t * uv_loop;
};


struct pomelo_statistic_platform_uv_s {
    /// @brief The number of scheduled timers
    size_t timers;

    /// @brief The number of scheduled works
    size_t worker_tasks;

    /// @brief The number of in-use deferred works
    size_t deferred_tasks;

    /// @brief The number of main tasks
    size_t threadsafe_tasks;

    /// @brief The number of using task groups
    size_t task_groups;

    /// @brief The number of in-use sending commands
    size_t send_commands;

    /// @brief The number of bytes which are sent by platform
    uint64_t sent_bytes;

    /// @brief The number of bytes which are recv by platform
    uint64_t recv_bytes;
};


/// @brief Create the UV platform
pomelo_platform_t * pomelo_platform_uv_create(
    pomelo_platform_uv_options_t * options
);


/// @brief Destroy the UV platform. After calling this function, if there are
/// some pending works to do, no more callbacks will be called.
void pomelo_platform_uv_destroy(pomelo_platform_t * platform);


/// @brief Get the UV loop of the platform
uv_loop_t * pomelo_platform_uv_get_uv_loop(pomelo_platform_t * platform);


/// @brief Get the statistic of platform
void pomelo_platform_uv_statistic(
    pomelo_platform_t * platform,
    pomelo_statistic_platform_uv_t * statistic
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_H
