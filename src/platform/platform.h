#ifndef POMELO_PLATFORM_SRC_H
#define POMELO_PLATFORM_SRC_H
#include <stdbool.h>
#include "pomelo/allocator.h"
#include "pomelo/address.h"
#include "pomelo/platform.h"
#include "pomelo/statistic/statistic-platform.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The platform socket
typedef struct pomelo_platform_udp_s pomelo_platform_udp_t;

/// @brief The platform timer
typedef struct pomelo_platform_timer_s pomelo_platform_timer_t;

/// @brief The buffer vector for sending
typedef struct pomelo_buffer_vector_s pomelo_buffer_vector_t;

/// @brief The group of tasks
typedef struct pomelo_platform_task_group_s pomelo_platform_task_group_t;

/// @brief The payload receiving callback
typedef void (*pomelo_platform_recv_cb)(
    void * context,
    pomelo_address_t * address,
    pomelo_buffer_vector_t * buffer,
    int status
);

/// @brief The payload sending callback
typedef void (*pomelo_platform_send_cb)(
    void * context,
    void * callback_data,
    int status
);

/// @brief The payload allocation callback for platform
typedef void (*pomelo_platform_alloc_cb)(
    void * context,
    pomelo_buffer_vector_t * buffer
);

/// @brief The timer callback
typedef void (*pomelo_platform_timer_cb)(void * callback_data);


struct pomelo_buffer_vector_s {
    /// @brief The data pointer
    uint8_t * data;

    /// @brief The length of pointer
    size_t length;
};


/* -------------------------------------------------------------------------- */
/*                            Platform Task APIs                              */
/* -------------------------------------------------------------------------- */

/// @brief Create new task group
pomelo_platform_task_group_t * pomelo_platform_acquire_task_group(
    pomelo_platform_t * platform
);


/// @brief Release a task group.
/// This will cancel all tasks of this group as well
void pomelo_platform_release_task_group(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group
);


/// @brief Cancel all pending tasks of a group.
/// Callbacks of deferred tasks will not be called.
/// Done callbacks of worker tasks will be called with `canceled = true`
/// @param callback The callback after stopping all works
/// @param callback_data The data which will be passed to the callback
int pomelo_platform_cancel_task_group(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb callback,
    void * callback_data
);


/// @brief Call the callback in the next tick.
/// This is useful when the calling function has to get out of current calling
/// stack
int pomelo_platform_submit_deferred_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb callback,
    void * callback_data
);


/// @brief Submit a work to run in threadpool with group
int pomelo_platform_submit_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb entry,
    pomelo_platform_task_done_cb done,
    void * callback_data
);


/* -------------------------------------------------------------------------- */
/*                             Platform UDP APIs                              */
/* -------------------------------------------------------------------------- */

/// @brief Bind the socket with specific address
/// @param platform The socket platform
/// @param address The address to bind
/// @return Pointer to platform socket or NULL if failed
pomelo_platform_udp_t * pomelo_platform_udp_bind(
    pomelo_platform_t * platform,
    pomelo_address_t * address
);

/// @brief Connect the socket to specific address
/// @param platform The socket platform
/// @param address The address to connect
/// @return Pointer to platform socket or NULL if failed
pomelo_platform_udp_t * pomelo_platform_udp_connect(
    pomelo_platform_t * platform,
    pomelo_address_t * address
);

/// @brief Stop the socket
int pomelo_platform_udp_stop(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket
);

/// @brief Send a packet to target
/// @param callback_data The data which will be passed to send_callback
int pomelo_platform_udp_send(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int nbuffers,
    pomelo_buffer_vector_t * buffers,
    void * callback_data
);

/// @brief Set the socket callback for platform
void pomelo_platform_udp_set_callbacks(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback,
    pomelo_platform_send_cb send_callback
);


/* -------------------------------------------------------------------------- */
/*                            Platform Timer APIs                             */
/* -------------------------------------------------------------------------- */

/// @brief Start the timer
/// @return New timer handle or NULL on failure
pomelo_platform_timer_t * pomelo_platform_timer_start(
    pomelo_platform_t * platform,
    pomelo_platform_timer_cb callback,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * callback_data
);

/// @brief Stop the timer
/// @return 0 on success or an error code < 0 on failure
int pomelo_platform_timer_stop(
    pomelo_platform_t * platform,
    pomelo_platform_timer_t * timer
);


/* -------------------------------------------------------------------------- */
/*                           Platform statisic APIs                           */
/* -------------------------------------------------------------------------- */

/// @brief Get the platform statistic
void pomelo_platform_statistic(
    pomelo_platform_t * platform,
    pomelo_statistic_platform_t * statistic
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_SRC_H
