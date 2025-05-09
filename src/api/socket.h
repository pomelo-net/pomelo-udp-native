#ifndef POMELO_API_SOCKET_SRC_H
#define POMELO_API_SOCKET_SRC_H
#include "pomelo/api.h"
#include "pomelo/constants.h"
#include "base/extra.h"
#include "base/sequencer.h"
#include "protocol/protocol.h"
#include "delivery/delivery.h"
#include "utils/array.h"
#include "utils/list.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief The callback function for disconnected event
typedef void (*pomelo_socket_on_disconnected_finalize_cb)(
    pomelo_socket_t * socket,
    pomelo_session_t * session
);


struct pomelo_socket_s {
    /// @brief The extra data
    pomelo_extra_t extra;

    /// @brief The flags of socket
    uint32_t flags;

    /// @brief The context
    pomelo_context_t * context;

    /// @brief The platform
    pomelo_platform_t * platform;

    /// @brief The protocol socket
    pomelo_protocol_socket_t * protocol_socket;

    /// @brief The state of socket
    pomelo_socket_state state;

    /// @brief List of sessions
    pomelo_list_t * sessions;

    /// @brief The flag store the stopped components
    uint8_t stopped_components;

    /// @brief The array of channel modes
    pomelo_array_t * channel_modes;

    /// @brief The counter for session signature
    uint64_t session_signature_generator;

    /// @brief The private key for server
    uint8_t private_key[POMELO_KEY_BYTES];

    /// @brief The adapter of socket
    pomelo_adapter_t * adapter;

    /// @brief The heartbeat
    pomelo_delivery_heartbeat_t * heartbeat;

    /// @brief The sequencer
    pomelo_sequencer_t sequencer;

    /// @brief The stop task of socket
    pomelo_sequencer_task_t stop_task;

    /// @brief The destroy task of socket
    pomelo_sequencer_task_t destroy_task;
};


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief On allocated callback
int pomelo_socket_on_alloc(
    pomelo_socket_t * socket,
    pomelo_context_t * context
);


/// @brief On free callback
void pomelo_socket_on_free(pomelo_socket_t * socket);


/// @brief Initialize the socket
int pomelo_socket_init(
    pomelo_socket_t * socket,
    pomelo_socket_options_t * options
);


/// @brief Cleanup the socket
void pomelo_socket_cleanup(pomelo_socket_t * socket);


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


/// @brief Dispatch the send result
void pomelo_socket_dispatch_send_result(
    pomelo_socket_t * socket,
    pomelo_message_t * message
);


/// @brief Send message to builtin sessions
int pomelo_socket_send_builtin(
    pomelo_socket_t * socket,
    pomelo_message_t * message,
    size_t channel_index,
    pomelo_session_t ** sessions,
    size_t nsessions
);


/// @brief Send message to plugin sessions
size_t pomelo_socket_send_plugin(
    pomelo_socket_t * socket,
    pomelo_message_t * message,
    size_t channel_index,
    pomelo_session_t ** sessions,
    size_t nsessions
);


/// @brief The deferred destroy function
void pomelo_socket_destroy_deferred(pomelo_socket_t * socket);


/// @brief The deferred stop function
void pomelo_socket_stop_deferred(pomelo_socket_t * socket);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_SOCKET_SRC_H
