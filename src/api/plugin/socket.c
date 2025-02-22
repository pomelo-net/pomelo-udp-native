#include <assert.h>
#include "pomelo/errno.h"
#include "socket.h"
#include "api/socket.h"


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

size_t POMELO_PLUGIN_CALL pomelo_plugin_socket_get_nchannels(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    assert(plugin != NULL);
    assert(socket != NULL);
    if (!plugin || !socket) {
        return POMELO_ERR_PLUGIN_INVALID_ARG;
    }

    return socket->nchannels;
}


/// @brief Get channel mode of socket
pomelo_channel_mode POMELO_PLUGIN_CALL pomelo_plugin_socket_get_channel_mode(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    size_t channel_index
) {
    assert(plugin != NULL);
    assert(socket != NULL);
    if (!plugin || !socket) {
        return POMELO_ERR_PLUGIN_INVALID_ARG;
    }

    if (channel_index >= socket->nchannels) {
        return POMELO_ERR_PLUGIN_INVALID_ARG;
    }

    return socket->channel_modes[channel_index];
}


void POMELO_PLUGIN_CALL pomelo_plugin_socket_attach(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    // This function is called in main thread.
    // So we just process straigth forward

    assert(plugin != NULL);
    assert(socket != NULL);
    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;

    impl->attached_list_node =
        pomelo_list_push_back(socket->attached_plugins, plugin);
}


void POMELO_PLUGIN_CALL pomelo_plugin_socket_detach(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    (void) plugin;
    assert(socket != NULL);

    pomelo_plugin_socket_detach_command_t * command =
        pomelo_pool_acquire(socket->detach_commands_pool);
    if (!command) {
        return; // Failed to acquire command
    }

    pomelo_platform_submit_main_task(
        socket->platform,
        (pomelo_platform_task_cb) pomelo_plugin_socket_process_detach,
        command
    );
}


void pomelo_plugin_socket_process_detach(
    pomelo_plugin_socket_detach_command_t * command
) {
    assert(command != NULL);

    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) command->plugin;
    pomelo_socket_t * socket = command->socket;
    pomelo_pool_release(socket->detach_commands_pool, command);

    pomelo_list_remove(socket->attached_plugins, impl->attached_list_node);

    if (socket->state == POMELO_SOCKET_STATE_STOPPING) {
        if (pomelo_list_is_empty(socket->attached_plugins)) {
            pomelo_plugin_on_all_plugins_detached_socket(socket);
        }
    }
}


uint64_t POMELO_PLUGIN_CALL pomelo_plugin_socket_time(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    (void) plugin;
    assert(socket != NULL);
    return pomelo_socket_time(socket);
}
