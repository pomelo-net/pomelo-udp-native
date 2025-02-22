#ifndef POMELO_API_SOCKET_SRC_H
#define POMELO_API_SOCKET_SRC_H
#include "pomelo/api.h"
#include "protocol/protocol.h"
#include "delivery/delivery.h"
#include "utils/list.h"
#include "base/extra.h"


#define POMELO_SOCKET_COMPONENT_PROTOCOL_SOCKET       (1 << 0)
#define POMELO_SOCKET_COMPONENT_DELIVERY_TRANSPORTER  (1 << 1)
#define POMELO_SOCKET_COMPONENT_PLUGINS               (1 << 2)
#define POMELO_SOCKET_COMPONENT_ALL (                                          \
    POMELO_SOCKET_COMPONENT_PROTOCOL_SOCKET |                                  \
    POMELO_SOCKET_COMPONENT_DELIVERY_TRANSPORTER |                             \
    POMELO_SOCKET_COMPONENT_PLUGINS                                            \
)


#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_socket_s {
    /// @brief The extra data
    pomelo_extra_t extra;

    /// @brief The allocator of socket
    pomelo_allocator_t * allocator;

    /// @brief The context
    pomelo_context_t * context;

    /// @brief The platform
    pomelo_platform_t * platform;

    /// @brief Task group
    pomelo_platform_task_group_t * task_group;

    /// @brief The protocol socket
    pomelo_protocol_socket_t * protocol_socket;

    /// @brief The transporter
    pomelo_delivery_transporter_t * transporter;

    /// @brief The pool of built-in sessions
    pomelo_pool_t * builtin_session_pool;

    /// @brief The state of socket
    pomelo_socket_state state;

    /// @brief List of sessions
    pomelo_list_t * sessions;

    /// @brief The flag store the stopped components
    uint8_t stopped_components;

    /// @brief The number of channels
    size_t nchannels;

    /// @brief The array of channel modes
    pomelo_channel_mode * channel_modes;

    /// @brief The counter for session signature
    uint64_t session_signature_generator;

    /// @brief The private key for server
    uint8_t private_key[POMELO_KEY_BYTES];

    /* Plugin parts */

    /// @brief Attached plugins of this socket
    pomelo_list_t * attached_plugins;

    /// @brief Pool of detaching commands
    pomelo_pool_t * detach_commands_pool;
};


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Process after all components have been stopped
void pomelo_socket_stop_deferred(pomelo_socket_t * socket);


/// @brief Process when an component is stopped
void pomelo_socket_process_stopped_component(
    pomelo_socket_t * socket,
    uint8_t component_mask
);


/// @brief Process when the protocol socket has been stop without request
void pomelo_socket_process_stopped_component_unexpectedly(
    pomelo_socket_t * socket,
    uint8_t component_mask
);


/// @brief Add session to socket sessions list
void pomelo_socket_add_session(
    pomelo_socket_t * socket,
    pomelo_session_t * session
);


/// @brief Remove session from socket sessions list
void pomelo_socket_remove_session(
    pomelo_socket_t * socket,
    pomelo_session_t * session
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_SOCKET_SRC_H
