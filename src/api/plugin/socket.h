#ifndef POMELO_API_PLUGIN_SOCKET_SRC_H
#define POMELO_API_PLUGIN_SOCKET_SRC_H
#include "plugin.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Detaching command
typedef struct pomelo_plugin_socket_detach_command_s
    pomelo_plugin_socket_detach_command_t;


struct pomelo_plugin_socket_detach_command_s {
    /// @brief Detaching plugin
    pomelo_plugin_t * plugin;

    /// @brief Detached socket
    pomelo_socket_t * socket;
};


/* -------------------------------------------------------------------------- */
/*                               Exported APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Get number of channels
size_t POMELO_PLUGIN_CALL pomelo_plugin_socket_get_nchannels(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
);


/// @brief Get channel mode of socket
pomelo_channel_mode POMELO_PLUGIN_CALL pomelo_plugin_socket_get_channel_mode(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket,
    size_t channel_index
);

/// @brief Attach socket
void POMELO_PLUGIN_CALL pomelo_plugin_socket_attach(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
);

/// @brief Detach socket
void POMELO_PLUGIN_CALL pomelo_plugin_socket_detach(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
);

/// @brief Get time of socket
uint64_t POMELO_PLUGIN_CALL pomelo_plugin_socket_time(
    pomelo_plugin_t * plugin,
    pomelo_socket_t * socket
);


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_plugin_socket_process_detach(
    pomelo_plugin_socket_detach_command_t * command
);

#ifdef __cplusplus
}
#endif
#endif // ~POMELO_API_PLUGIN_SOCKET_SRC_H
