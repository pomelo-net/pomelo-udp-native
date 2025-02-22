#ifndef POMELO_PLATFORM_UV_TIMER_SRC_H
#define POMELO_PLATFORM_UV_TIMER_SRC_H
#include "uv.h"
#include "platform/platform.h"
#include "utils/pool.h"
#include "utils/list.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Timer controller
typedef struct pomelo_platform_timer_controller_s
    pomelo_platform_timer_controller_t;


struct pomelo_platform_timer_s {
    /// @brief The user data
    void * data;

    /// @brief The based platform
    pomelo_platform_timer_controller_t * controller;

    /// @brief The callback
    pomelo_platform_timer_cb callback;

    /// @brief The timer
    uv_timer_t uv_timer;

    /// @brief The repeat flag
    bool is_repeat;

    /// @brief The running flag
    bool is_running;

    /// @brief The node of this timer in the controlled list of the controller
    pomelo_list_node_t * list_node;
};


struct pomelo_platform_timer_controller_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief UV loop
    uv_loop_t * uv_loop;

    /// @brief The timer pool
    pomelo_pool_t * timer_pool;

    /// @brief All active timers
    pomelo_list_t * timers;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Create the timer controller
pomelo_platform_timer_controller_t * pomelo_platform_timer_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
);


/// @brief Destroy the timer controller
void pomelo_platform_timer_controller_destroy(
    pomelo_platform_timer_controller_t * controller
);


/// @brief Get the statistic of timer controller
void pomelo_platform_timer_controller_statistic(
    pomelo_platform_timer_controller_t * controller,
    pomelo_statistic_platform_t * statistic
);


/// @brief Startup the timer controller
void pomelo_platform_timer_controller_startup(
    pomelo_platform_timer_controller_t * controller
);


/// @brief Shutdown the timer controller
void pomelo_platform_timer_controller_shutdown(
    pomelo_platform_timer_controller_t * controller
);


/// @brief Start a timer
pomelo_platform_timer_t * pomelo_platform_timer_controller_start(
    pomelo_platform_timer_controller_t * controller,
    pomelo_platform_timer_cb callback,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * callback_data
);


/// @brief Stop the timer
int pomelo_platform_timer_controller_stop(
    pomelo_platform_timer_controller_t * controller,
    pomelo_platform_timer_t * timer
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Stop timer
int pomelo_platform_timer_stop_ex(
    pomelo_platform_timer_controller_t * controller,
    pomelo_platform_timer_t * timer
);


/// @brief The callback from uv timer
void pomelo_platform_uv_timer_callback(uv_timer_t * uv_timer);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_UV_TIMER_SRC_H
