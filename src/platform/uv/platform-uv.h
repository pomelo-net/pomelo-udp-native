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

/// @brief Platform UV
typedef struct pomelo_platform_uv_s pomelo_platform_uv_t;

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


struct pomelo_platform_uv_s {
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

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Set the extra data of platform
void pomelo_platform_uv_set_extra(pomelo_platform_uv_t * platform, void * data);


/// @brief Get the extra data of platform
void * pomelo_platform_uv_get_extra(pomelo_platform_uv_t * platform);


/// @brief Startup the platform
void pomelo_platform_uv_startup(pomelo_platform_uv_t * platform);


/// @brief Shutdown the platform
void pomelo_platform_uv_shutdown(
    pomelo_platform_uv_t * platform,
    pomelo_platform_shutdown_callback callback
);


/// @brief Acquire a threadsafe executor
pomelo_threadsafe_executor_t * pomelo_platform_uv_acquire_threadsafe_executor(
    pomelo_platform_uv_t * platform
);


/// @brief Release a threadsafe executor
void pomelo_platform_uv_release_threadsafe_executor(
    pomelo_platform_uv_t * platform,
    pomelo_threadsafe_executor_t * executor
);


/// @brief Submit a task to threadsafe executor
pomelo_platform_task_t * pomelo_threadsafe_executor_uv_submit(
    pomelo_platform_uv_t * platform,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
);


/// @brief Get high resolution time
uint64_t pomelo_platform_uv_hrtime(pomelo_platform_uv_t * platform);


/// @brief Get current time
uint64_t pomelo_platform_uv_now(pomelo_platform_uv_t * platform);


/// @brief Start a timer
int pomelo_platform_uv_timer_start(
    pomelo_platform_uv_t * platform,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
);


/// @brief Stop a timer
void pomelo_platform_uv_timer_stop(
    pomelo_platform_uv_t * platform,
    pomelo_platform_timer_handle_t * handle
);


/// @brief Bind a UDP socket
pomelo_platform_udp_t * pomelo_platform_uv_udp_bind(
    pomelo_platform_uv_t * platform,
    pomelo_address_t * address
);


/// @brief Connect a UDP socket
pomelo_platform_udp_t * pomelo_platform_uv_udp_connect(
    pomelo_platform_uv_t * platform,
    pomelo_address_t * address
);


/// @brief Stop a UDP socket
int pomelo_platform_uv_udp_stop(
    pomelo_platform_uv_t * platform,
    pomelo_platform_udp_t * socket
);


/// @brief Send data to a UDP socket
int pomelo_platform_uv_udp_send(
    pomelo_platform_uv_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int nbuffers,
    pomelo_platform_iovec_t * buffers,
    void * callback_data,
    pomelo_platform_send_cb send_callback
);


/// @brief Start receiving packets from socket
void pomelo_platform_uv_udp_recv_start(
    pomelo_platform_uv_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback
);


/// @brief Submit a worker task
pomelo_platform_task_t * pomelo_platform_uv_submit_worker_task(
    pomelo_platform_uv_t * platform,
    pomelo_platform_task_entry entry,
    pomelo_platform_task_complete complete,
    void * data
);


/// @brief Cancel a worker task
void pomelo_platform_uv_cancel_worker_task(
    pomelo_platform_uv_t * platform,
    pomelo_platform_task_t * task
);


/* -------------------------------------------------------------------------- */
/*                              Private APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Check if the platform is shutdown
void pomelo_platform_check_shutdown(pomelo_platform_uv_t * platform);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_SRC_H
