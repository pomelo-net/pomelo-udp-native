#ifndef POMELO_PLATFORM_UV_SRC_H
#define POMELO_PLATFORM_UV_SRC_H
#include "uv.h"
#include "pomelo/platforms/platform-uv.h"
#include "base/extra.h"
#include "platform/platform.h"
#include "platform-task-main.h"
#include "platform-task-worker.h"
#include "platform-task-group.h"
#include "platform-task-deferred.h"
#include "platform/common/platform-udp.h"
#include "platform/common/platform-timer.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_platform_s {
    /// @brief The extra data
    pomelo_extra_t extra;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The uv loop
    uv_loop_t * uv_loop;

    /// @brief The timer manager
    pomelo_platform_timer_controller_t * timer_controller;

    /// @brief The socket manager
    pomelo_platform_udp_controller_t * udp_controller;

    /// @brief Deferred task controller
    pomelo_platform_task_deferred_controller_t * task_deferred_controller;
    
    /// @brief Worker task controller
    pomelo_platform_task_worker_controller_t * task_worker_controller;

    /// @brief Main task controller
    pomelo_platform_task_main_controller_t * task_main_controller;

    /// @brief Pool of task groups
    pomelo_pool_t * task_group_pool;
};


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_SRC_H
