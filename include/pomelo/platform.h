#ifndef POMELO_PLATFORM_H
#define POMELO_PLATFORM_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/// @brief Platform interface for managing network and system resources.
/// 
/// The platform layer provides core functionality for:
/// - Network socket management and I/O
/// - Task scheduling and execution
/// - Timer management
/// - System time access
///
/// To safely execute code from other threads, use the threadsafe executor
/// APIs like `pomelo_platform_threadsafe_executor_submit()` to submit tasks
/// that will run on the main platform thread.
typedef struct pomelo_platform_s pomelo_platform_t;

/// @brief The task
typedef struct pomelo_platform_task_s pomelo_platform_task_t;

/// @brief The threadsafe executor interface
///
/// This interface is used to submit tasks to the platform thread.
/// It is threadsafe and can be called from any thread.
/// 
/// The tasks will be executed in the platform thread.
typedef struct pomelo_threadsafe_executor_s pomelo_threadsafe_executor_t;

/// @brief The entry point of task
typedef void (*pomelo_platform_task_entry)(void * data);

/// @brief The shutdown callback
typedef void (*pomelo_platform_shutdown_callback)(pomelo_platform_t * platform);


/* -------------------------------------------------------------------------- */
/*                               Common APIs                                  */
/* -------------------------------------------------------------------------- */


/// @brief Set the platform extra data
void pomelo_platform_set_extra(pomelo_platform_t * platform, void * data);


/// @brief Get the platform extra data
void * pomelo_platform_get_extra(pomelo_platform_t * platform);


/// @brief Startup the platform
void pomelo_platform_startup(pomelo_platform_t * platform);


/// @brief Shutdown the platform. The callback will be called when the platform
/// is completely shutdown.
void pomelo_platform_shutdown(
    pomelo_platform_t * platform,
    pomelo_platform_shutdown_callback callback
);


/* -------------------------------------------------------------------------- */
/*                                 Time APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Get the high-resolution time (nanoseconds) (Threadsafe)
uint64_t pomelo_platform_hrtime(pomelo_platform_t * platform);


/// @brief Get the time in unix timestamp (milliseconds) (Threadsafe)
uint64_t pomelo_platform_now(pomelo_platform_t * platform);


/* -------------------------------------------------------------------------- */
/*                           Threadsafe executor APIs                         */
/* -------------------------------------------------------------------------- */

/// @brief Acquire a threadsafe executor (Non-threadsafe).
/// When platform is shutdown, the threadsafe executor will be released.
pomelo_threadsafe_executor_t * pomelo_platform_acquire_threadsafe_executor(
    pomelo_platform_t * platform
);


/// @brief Release the threadsafe executor (Threadsafe).
void pomelo_platform_release_threadsafe_executor(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor
);


/// @brief Submit a task to run in platform thread (Threadsafe)
pomelo_platform_task_t * pomelo_threadsafe_executor_submit(
    pomelo_platform_t * platform,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PLATFORM_H
