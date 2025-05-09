#ifndef POMELO_PLATFORM_UV_SRC_H
#define POMELO_PLATFORM_UV_SRC_H
#include "uv.h"
#include "pomelo/platforms/platform-uv.h"
#include "base/extra.h"
#include "platform/platform.h"

#ifdef __cplusplus
extern "C" {
#endif


#define POMELO_PLATFORM_UV_COMPONENT_TIMER      (1 << 0)
#define POMELO_PLATFORM_UV_COMPONENT_UDP        (1 << 1)
#define POMELO_PLATFORM_UV_COMPONENT_WORKER     (1 << 2)
#define POMELO_PLATFORM_UV_COMPONENT_THREADSAFE (1 << 3)
#define POMELO_PLATFORM_UV_COMPONENT_ALL (    \
    POMELO_PLATFORM_UV_COMPONENT_TIMER      | \
    POMELO_PLATFORM_UV_COMPONENT_UDP        | \
    POMELO_PLATFORM_UV_COMPONENT_WORKER     | \
    POMELO_PLATFORM_UV_COMPONENT_THREADSAFE   \
)

/// @brief The timer controller
typedef struct pomelo_platform_timer_controller_s
    pomelo_platform_timer_controller_t;

/// @brief The udp controller
typedef struct pomelo_platform_udp_controller_s
    pomelo_platform_udp_controller_t;

/// @brief The worker controller
typedef struct pomelo_platform_worker_controller_s
    pomelo_platform_worker_controller_t;

/// @brief The threadsafe controller
typedef struct pomelo_platform_threadsafe_controller_s
    pomelo_platform_threadsafe_controller_t;


struct pomelo_platform_s {
    /// @brief The extra data
    pomelo_extra_t extra;

    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The uv loop
    uv_loop_t * uv_loop;

    /// @brief The flag of running
    bool running;

    /// @brief The idle handle for shutdown
    uv_idle_t shutdown_idle;

    /// @brief The shutdown callback
    pomelo_platform_shutdown_callback shutdown_callback;

    /// @brief The bitmask of shutdown components
    uint32_t shutdown_components;

    /// @brief The timer manager
    pomelo_platform_timer_controller_t * timer_controller;

    /// @brief The socket manager
    pomelo_platform_udp_controller_t * udp_controller;

    /// @brief Worker controller
    pomelo_platform_worker_controller_t * worker_controller;

    /// @brief Threadsafe controller
    pomelo_platform_threadsafe_controller_t * threadsafe_controller;
};


/// @brief Check if the platform is shutdown
void pomelo_platform_check_shutdown(pomelo_platform_t * platform);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_SRC_H
