#ifndef POMELO_STATISTIC_PLATFORM_H
#define POMELO_STATISTIC_PLATFORM_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_statistic_platform_s {
    /// @brief The number of scheduled timers
    size_t timers;

    /// @brief The number of scheduled works
    size_t worker_tasks;

    /// @brief The number of in-use deferred works
    size_t deferred_tasks;

    /// @brief The number of main tasks
    size_t main_tasks;

    /// @brief The number of using task groups
    size_t task_groups;

    /// @brief The number of in-use sending commands
    size_t send_commands;

    /// @brief The number of bytes which are sent by platform
    uint64_t sent_bytes;

    /// @brief The number of bytes which are recv by platform
    uint64_t recv_bytes;
};


/// @brief The statistic of platform module
typedef struct pomelo_statistic_platform_s pomelo_statistic_platform_t;


#ifdef __cplusplus
}
#endif
#endif // POMELO_STATISTIC_PLATFORM_H
