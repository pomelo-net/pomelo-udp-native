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
    if (!plugin || !socket) return 0;

    pomelo_plugin_check_signature((pomelo_plugin_impl_t *) plugin);
    return socket->channel_modes->size;
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
    pomelo_plugin_check_signature((pomelo_plugin_impl_t *) plugin);

    if (channel_index >= socket->channel_modes->size) {
        return POMELO_ERR_PLUGIN_INVALID_ARG;
    }

    pomelo_channel_mode mode;
    pomelo_array_get(socket->channel_modes, channel_index, &mode);
    return mode;
}


uint64_t POMELO_PLUGIN_CALL pomelo_plugin_socket_time(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
) {
    assert(plugin != NULL);
    assert(socket != NULL);
    pomelo_plugin_impl_t * impl = (pomelo_plugin_impl_t *) plugin;
    pomelo_plugin_check_signature(impl);

    return pomelo_socket_time(socket);
}
