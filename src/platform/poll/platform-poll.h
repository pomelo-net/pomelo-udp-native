#ifndef POMELO_PLATFORM_POLL_SRC_H
#define POMELO_PLATFORM_POLL_SRC_H
#include "uv.h"
#include "pomelo/platforms/platform-poll.h"
#include "base/extra.h"
#include "platform/platform.h"
#include "platform/common/platform-udp.h"
#include "platform/common/platform-timer.h"
#include "platform-task.h"
#ifdef __cplusplus
extern "C" {
#endif


/* This platform reuse some controllers from platform UV */


struct pomelo_platform_s {
    /// @brief Extra data
    pomelo_extra_t extra;

    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief This platform has its own uv loop
    uv_loop_t uv_loop;

    /// @brief The timer manager
    pomelo_platform_timer_controller_t * timer_controller;

    /// @brief The socket manager
    pomelo_platform_udp_controller_t * udp_controller;

    /// @brief The task controller
    pomelo_platform_task_controller_t * task_controller;

    /// @brief Pool of task groups
    pomelo_pool_t * task_group_pool;
};


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_POLL_SRC_H
