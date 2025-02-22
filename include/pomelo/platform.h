#ifndef POMELO_PLATFORM_H
#define POMELO_PLATFORM_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif



/// @brief Platform.
/// Platforms manage sockets, works, timers and tasks.
/// Pomelo itself is not thread-safe, so that, calling any APIs in other threads 
/// may crash the program.
/// In order to submit a task to pomelo thread,
/// use `pomelo_platform_submit_main_task`
typedef struct pomelo_platform_s pomelo_platform_t;

/// @brief The callback for task
typedef void (*pomelo_platform_task_cb)(void * data);

/// @brief The done point of work which will be run after entry and in main
/// thread
typedef void (*pomelo_platform_task_done_cb)(void * data, bool canceled);


/// @brief Set the platform extra data
void pomelo_platform_set_extra(pomelo_platform_t * platform, void * data);

/// @brief Get the platform extra data
void * pomelo_platform_get_extra(pomelo_platform_t * platform);


/* -------------------------------------------------------------------------- */
/*                              Common APIs                                   */
/* -------------------------------------------------------------------------- */

/// @brief Startup the platform (Non-threadsafe)
void pomelo_platform_startup(pomelo_platform_t * platform);

/// @brief Shutdown the platform (Non-threadsafe)
void pomelo_platform_shutdown(pomelo_platform_t * platform);


/* -------------------------------------------------------------------------- */
/*                               Time APIs                                    */
/* -------------------------------------------------------------------------- */

/// @brief Get the high-resolution time (nanoseconds) (Threadsafe)
uint64_t pomelo_platform_hrtime(pomelo_platform_t * platform);

/// @brief Get the time in unix timestamp (milliseconds) (Threadsafe)
uint64_t pomelo_platform_now(pomelo_platform_t * platform);


/* -------------------------------------------------------------------------- */
/*                                Task APIs                                   */
/* -------------------------------------------------------------------------- */

/// @brief Submit a task to run in platform thread (Threadsafe)
/// If the platform job has not been started up, this function will not work
/// and -1 will be returned.
int pomelo_platform_submit_main_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_cb callback,
    void * callback_data
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_H
