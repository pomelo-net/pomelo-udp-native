#ifndef POMELO_PLUGIN_EXECUTOR_SRC_H
#define POMELO_PLUGIN_EXECUTOR_SRC_H
#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The command for the thread-safe executor
typedef struct pomelo_plugin_executor_command_s
    pomelo_plugin_executor_command_t;


struct pomelo_plugin_executor_command_s {
    /// @brief The plugin that owns the command
    pomelo_plugin_t * plugin;

    /// @brief The callback of the command
    pomelo_plugin_task_callback callback;

    /// @brief The data of the command
    void * data;
};


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */


/// @brief Startup the thread-safe executor
int POMELO_PLUGIN_CALL pomelo_plugin_executor_startup(pomelo_plugin_t * plugin);


/// @brief Shutdown the thread-safe executor
void POMELO_PLUGIN_CALL pomelo_plugin_executor_shutdown(
    pomelo_plugin_t * plugin
);


/// @brief Submit a task to the threadsafe executor
int POMELO_PLUGIN_CALL pomelo_plugin_executor_submit(
    pomelo_plugin_t * plugin,
    pomelo_plugin_task_callback callback,
    void * data
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_PLUGIN_EXECUTOR_SRC_H
