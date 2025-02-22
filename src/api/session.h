#ifndef POMELO_API_SESSION_SRC_H
#define POMELO_API_SESSION_SRC_H
#include "pomelo/api.h"
#include "delivery/delivery.h"
#include "protocol/protocol.h"
#include "base/extra.h"
#include "utils/list.h"
#ifdef POMELO_MULTI_THREAD
#include "utils/atomic.h"
#endif // POMELO_MULTI_THREAD

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Session methods table
typedef struct pomelo_session_methods_s pomelo_session_methods_t;


/// @brief Disconnecting function of session
typedef int (*pomelo_session_disconnect_fn)(pomelo_session_t * session);


/// @brief Getting RTT function of session
typedef int (*pomelo_session_get_rtt_fn)(
    pomelo_session_t * session,
    uint64_t * mean,
    uint64_t * variance
);


/// @brief Getting channel function of session
typedef pomelo_channel_t * (*pomelo_session_get_channel_fn)(
    pomelo_session_t * session,
    size_t channel_index
);


/// @brief Release session
typedef void (*pomelo_session_release_fn)(pomelo_session_t * session);

#ifdef POMELO_MULTI_THREAD
typedef pomelo_atomic_uint64_t pomelo_session_signature_t;
#else // !POMELO_MULTI_THREAD
typedef uint64_t pomelo_session_signature_t;
#endif // POMELO_MULTI_THREAD

struct pomelo_session_methods_s {
    /// @brief Disconnecting method
    pomelo_session_disconnect_fn disconnect;

    /// @brief Getting RTT method
    pomelo_session_get_rtt_fn get_rtt;

    /// @brief Getting channel method
    pomelo_session_get_channel_fn get_channel;

    /// @brief Release session method
    pomelo_session_release_fn release;
};


struct pomelo_session_s {
    /// @brief The extra data of session
    pomelo_extra_t extra;

    /// @brief The socket
    pomelo_socket_t * socket;

    /// @brief The ID of session
    int64_t client_id;

    /// @brief The address of session
    pomelo_address_t address;

    /// @brief The signature of session.
    pomelo_session_signature_t signature;

    /// @brief Methods table of session
    pomelo_session_methods_t * methods;

    /// @brief Node of session in list
    pomelo_list_node_t * node;
};

/// @brief Initialize base session
int pomelo_session_init(pomelo_session_t * session, pomelo_socket_t * socket);

/// @brief FInalize base session
int pomelo_session_finalize(
    pomelo_session_t * session,
    pomelo_socket_t * socket
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_API_SESSION_SRC_H
