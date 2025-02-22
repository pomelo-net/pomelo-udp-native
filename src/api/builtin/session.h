#ifndef POMELO_API_BUILTIN_SESSION_SRC_H
#define POMELO_API_BUILTIN_SESSION_SRC_H
#include "api/session.h"
#include "delivery/delivery.h"
#include "protocol/protocol.h"
#include "builtin.h"


#ifdef __cplusplus
extern "C" {
#endif

struct pomelo_session_builtin_s {
    /// @brief The base session
    pomelo_session_t base;

    /// @brief The delivery endpoint
    pomelo_delivery_endpoint_t * endpoint;

    /// @brief The protocol peer
    pomelo_protocol_peer_t * peer;

    /// @brief The array of channels
    pomelo_channel_builtin_t * channels;
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


/// @brief Wrap the endpoint and peer with this session
int pomelo_session_builtin_wrap(
    pomelo_session_builtin_t * session,
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_protocol_peer_t * peer
);


/// @brief Pool-compatible acquiring callback
int pomelo_session_builtin_init(
    pomelo_session_builtin_t * session,
    pomelo_socket_t * socket
);


/// @brief Pool-compatible releasing callback
int pomelo_session_builtin_finalize(
    pomelo_session_builtin_t * session,
    pomelo_socket_t * socket
);


/// @brief Release built-in session
void pomelo_session_builtin_release(pomelo_session_builtin_t * session);


/// @brief Process disconnected event of builtin session
void pomelo_session_builtin_on_disconnected(pomelo_session_builtin_t * session);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_BUILTIN_SESSION_SRC_H
