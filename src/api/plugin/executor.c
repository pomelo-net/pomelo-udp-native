#include <assert.h>
#include "executor.h"


int POMELO_PLUGIN_CALL pomelo_plugin_executor_startup(
    pomelo_plugin_t * plugin
) {
    assert(plugin != NULL);
    if (!plugin) return -1; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    pomelo_platform_t * platform = impl->platform;

    if (impl->executor) return 0; // Already started

    impl->executor = pomelo_platform_acquire_threadsafe_executor(platform);
    if (!impl->executor) return -1; // Failed to acquire executor

    return 0;
}


void POMELO_PLUGIN_CALL pomelo_plugin_executor_shutdown(
    pomelo_plugin_t * plugin
) {
    assert(plugin != NULL);
    if (!plugin) return; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    pomelo_platform_t * platform = impl->platform;

    if (!impl->executor) return; // Not started

    pomelo_platform_release_threadsafe_executor(platform, impl->executor);
    impl->executor = NULL;
}


static void executor_command_callback(
    pomelo_plugin_executor_command_t * command
) {
    assert(command != NULL);

    pomelo_plugin_task_callback callback = command->callback;
    void * data = command->data;
    pomelo_plugin_t * plugin = command->plugin;
    
    // Release the command
    pomelo_pool_release(
        ((pomelo_plugin_impl_t *) plugin)->command_pool,
        command
    );

    // Finally, execute the callback
    callback(plugin, data);
}


int POMELO_PLUGIN_CALL pomelo_plugin_executor_submit(
    pomelo_plugin_t * plugin,
    pomelo_plugin_task_callback callback,
    void * data
) {
    assert(plugin != NULL);
    assert(callback != NULL);
    (void) data;
    if (!plugin || !callback) return -1; // Invalid arguments

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    if (!impl->executor) return -1; // Not started

    pomelo_plugin_executor_command_t * command =
        pomelo_pool_acquire(impl->command_pool, NULL);
    if (!command) return -1; // Failed to acquire command

    command->plugin = plugin;
    command->callback = callback;
    command->data = data;

    pomelo_threadsafe_executor_submit(
        impl->platform,
        impl->executor,
        (pomelo_platform_task_entry) executor_command_callback,
        command
    );

    return 0;
}
