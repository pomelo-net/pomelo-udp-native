#ifndef POMELO_API_BUILTIN_SESSION_SRC_H
#define POMELO_API_BUILTIN_SESSION_SRC_H
#include "utils/array.h"
#include "api/session.h"
#include "delivery/delivery.h"
#include "protocol/protocol.h"
#include "builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/// @brief The session builtin info
typedef struct pomelo_session_builtin_info_s pomelo_session_builtin_info_t;


struct pomelo_session_builtin_info_s {
    /// @brief The socket
    pomelo_socket_t * socket;

    /// @brief The protocol peer
    pomelo_protocol_peer_t * peer;
};


struct pomelo_session_builtin_s {
    /// @brief The base session
    pomelo_session_t base;

    /// @brief The delivery endpoint
    pomelo_delivery_endpoint_t * endpoint;

    /// @brief The protocol peer
    pomelo_protocol_peer_t * peer;

    /// @brief The array of channels
    pomelo_array_t * channels;

    /// @brief The on disconnected task
    pomelo_sequencer_task_t on_disconnected_task;

    /// @brief Whether the session is ready
    bool ready;
};


/// @brief Get the builtin session methods table
pomelo_session_methods_t * pomelo_session_builtin_methods(void);


/// @brief Disconnect builtin session
int pomelo_session_builtin_disconnect(pomelo_session_builtin_t * session);


/// @brief Get rtt of builtin session
int pomelo_session_builtin_get_rtt(
    pomelo_session_builtin_t * session,
    uint64_t * mean,
    uint64_t * variance
);


/// @brief Get channel of builtin session
pomelo_channel_builtin_t * pomelo_session_builtin_get_channel(
    pomelo_session_builtin_t * session,
    size_t channel_index
);


/// @brief On allocated callback
int pomelo_session_builtin_on_alloc(
    pomelo_session_builtin_t * session,
    pomelo_context_t * context
);


/// @brief On free callback
void pomelo_session_builtin_on_free(pomelo_session_builtin_t * session);


/// @brief Initialize the builtin session
int pomelo_session_builtin_init(
    pomelo_session_builtin_t * session,
    pomelo_session_builtin_info_t * info
);


/// @brief Cleanup the builtin session
void pomelo_session_builtin_cleanup(pomelo_session_builtin_t * session);


/// @brief Process disconnected event of builtin session
void pomelo_session_builtin_on_disconnected(pomelo_session_builtin_t * session);


/// @brief The deferred function of on disconnected event of builtin session
void pomelo_session_builtin_on_disconnected_deferred(
    pomelo_session_builtin_t * session
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_BUILTIN_SESSION_SRC_H
