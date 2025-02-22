#ifndef POMELO_PROTOCOL_SRC_H
#define POMELO_PROTOCOL_SRC_H
#include "pomelo/allocator.h"
#include "pomelo/address.h"
#include "pomelo/statistic/statistic-protocol.h"
#include "base/packet.h"
#include "base/buffer.h"
#include "platform/platform.h"

#ifdef __cplusplus
extern "C" {
#endif


/// @brief Connect result
typedef enum pomelo_protocol_connect_result_e {
    POMELO_PROTOCOL_SOCKET_CONNECT_TIMED_OUT = -2,
    POMELO_PROTOCOL_SOCKET_CONNECT_DENIED = -1,
    POMELO_PROTOCOL_SOCKET_CONNECT_SUCCESS = 0
} pomelo_protocol_connect_result;


/// @brief The interface of protocol context
typedef struct pomelo_protocol_context_s pomelo_protocol_context_t;

/// @brief The context for protocol
typedef struct pomelo_protocol_context_root_s pomelo_protocol_context_root_t;

#ifdef POMELO_MULTI_THREAD

/// @brief Shared context for a local thread
typedef struct pomelo_protocol_context_shared_s
    pomelo_protocol_context_shared_t;

typedef struct pomelo_protocol_context_shared_options_s
    pomelo_protocol_context_shared_options_t;

#endif // !POMELO_MULTI_THREAD

/// @brief The socket
typedef struct pomelo_protocol_socket_s pomelo_protocol_socket_t;

/// @brief The socket client
typedef struct pomelo_protocol_client_s pomelo_protocol_client_t;

/// @brief The socket server
typedef struct pomelo_protocol_server_s pomelo_protocol_server_t;

/// The peer. This struct represents the connection between 2 sockets:
/// client & server. So, send a packet from one side to this peer, the other
/// side will receive the message.
typedef struct pomelo_protocol_peer_s pomelo_protocol_peer_t;

/// @brief The protocol context creating options
typedef struct pomelo_protocol_context_root_options_s
    pomelo_protocol_context_root_options_t;

/// @brief The socket server creating options
typedef struct pomelo_protocol_server_options_s
    pomelo_protocol_server_options_t;

/// @brief The socket client creating options
typedef struct pomelo_protocol_client_options_s
    pomelo_protocol_client_options_t;


struct pomelo_protocol_context_root_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The buffer context
    pomelo_buffer_context_root_t * buffer_context;

    /// @brief The whole capacity of a packet
    size_t packet_capacity;
};


struct pomelo_protocol_context_s {
    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief The whole capacity of a packet
    size_t packet_capacity;

    /// @brief Maximum size of packet payload body.
    /// packet_payload_body_capacity = (packet_capacity - header_size)
    size_t packet_payload_body_capacity;
};


struct pomelo_protocol_context_shared_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The root context
    pomelo_protocol_context_root_t * context;
};


struct pomelo_protocol_server_options_s {
    /// @brief The allocator to be used in system.
    /// If NULL is set, socket will use the default allocator
    pomelo_allocator_t * allocator;

    /// @brief The platform which this socket relies on
    pomelo_platform_t * platform;

    /// @brief The protocol context
    pomelo_protocol_context_t * context;

    /// @brief The maximum number of sessions
    size_t max_clients;

    /// @brief The protocol ID specified by app/game
    int64_t protocol_id;

    /// @brief The private key
    uint8_t * private_key;

    /// @brief The bind address of socket
    pomelo_address_t address;
};


struct pomelo_protocol_client_options_s {
    /// @brief The allocator to be used in system.
    /// If NULL is set, socket will use the default allocator
    pomelo_allocator_t * allocator;

    /// @brief The platform which this socket relies on
    pomelo_platform_t * platform;

    /// @brief The protocol context
    pomelo_protocol_context_t * context;

    /// @brief The connect token
    uint8_t * connect_token;
};


/* -------------------------------------------------------------------------- */
/*                               Context APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Set the default options
/// @param options The options
/// @return 0 on success, or an error code < 0 on failure. 
int pomelo_protocol_context_root_options_init(
    pomelo_protocol_context_root_options_t * options
);


/// @brief Create new protocol context
/// @param options The creating options
/// @return New protocol context or NULl on failure
pomelo_protocol_context_root_t * pomelo_protocol_context_root_create(
    pomelo_protocol_context_root_options_t * options
);


/// @brief Destroy an unused protocol context
/// @param context The context to destroy
void pomelo_protocol_context_root_destroy(
    pomelo_protocol_context_root_t * context
);


/* -------------------------------------------------------------------------- */
/*                             Shared Context APIs                            */
/* -------------------------------------------------------------------------- */

#ifdef POMELO_MULTI_THREAD

/// @brief Initialize the creating options for local-thread protocol context
int pomelo_protocol_context_shared_options_init(
    pomelo_protocol_context_shared_options_t * options
);

/// @brief Create local thread context
pomelo_protocol_context_shared_t * pomelo_protocol_context_shared_create(
    pomelo_protocol_context_shared_options_t * options
);

/// @brief Destroy the local thread
void pomelo_protocol_context_shared_destroy(
    pomelo_protocol_context_shared_t * context
);

#endif // !POMELO_MULTI_THREAD


/* -------------------------------------------------------------------------- */
/*                                Socket APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Set the default options
/// @param options The options
/// @return 0 on success, or an error code < 0 on failure. 
int pomelo_protocol_server_options_init(
    pomelo_protocol_server_options_t * options
);


/// @brief Set the default options
/// @param options The options
/// @return 0 on success, or an error code < 0 on failure. 
int pomelo_protocol_client_options_init(
    pomelo_protocol_client_options_t * options
);


/// @brief Create new socket server
/// @param options The socket options
/// @return Returns new socket or NULL if failed
pomelo_protocol_socket_t * pomelo_protocol_server_create(
    pomelo_protocol_server_options_t * options
);


/// @brief Create new socket client
/// @param options The socket options
/// @return Returns new socket or NULL if failed
pomelo_protocol_socket_t * pomelo_protocol_client_create(
    pomelo_protocol_client_options_t * options
);


/// @brief Destroy the socket.
/// Destroying a running socket will cause unexpected behaviors.
/// Remember to stop the socket before destroying it.
/// @param socket The socket
void pomelo_protocol_socket_destroy(pomelo_protocol_socket_t * socket);


/// @brief Start the socket
/// @param socket The socket
/// @return 0 on success, or an error code < 0 on failure.
int pomelo_protocol_socket_start(pomelo_protocol_socket_t * socket);


/// @brief Stop listening or connecting.
/// The socket will take a while to completely stop working.
/// Then, the callback pomelo_protocol_on_socket_stopped will be called.
/// @param socket The socket
/// @return 0 on success, or an error code < 0 on failure. 
void pomelo_protocol_socket_stop(pomelo_protocol_socket_t * socket);


/// @brief Get the socket extra data
#define pomelo_protocol_socket_get_extra(socket) (*((void **) (socket)))


/// @brief Set the socket data for socket
#define pomelo_protocol_socket_set_extra(socket, data)                         \
    (*((void **) (socket)) = (data))


/// @brief Get the statistic of socket protocol
void pomelo_protocol_socket_statistic(
    pomelo_protocol_socket_t * socket,
    pomelo_statistic_protocol_t * statistic
);


/// @brief Get the socket synchronized time
uint64_t pomelo_protocol_socket_time(pomelo_protocol_socket_t * socket);


/* -------------------------------------------------------------------------- */
/*                              Socket peer APIs                              */
/* -------------------------------------------------------------------------- */


/// @brief Get the client ID of peer
int64_t pomelo_protocol_peer_get_client_id(pomelo_protocol_peer_t * peer);


/// @brief Get the peer address
pomelo_address_t * pomelo_protocol_peer_get_address(
    pomelo_protocol_peer_t * peer
);


/// @brief Send payload.
/// The payload will be encrypted (if it has not), so that, the content will be
/// changed.
/// Do not modify the payload after calling this function.
/// @param peer The target peer
/// @param buffer The buffer to send
/// @param offset Offset of buffer to send
/// @param length Length of data
int pomelo_protocol_peer_send(
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
);


/// @brief Get the peer extra data
#define pomelo_protocol_peer_get_extra(peer) (*((void **) (peer)))

/// @brief Set the extra data for peer
#define pomelo_protocol_peer_set_extra(peer, data)                             \
    (*((void **) (peer)) = (data))

/// @brief Disconnect a peer.
/// If this is the peer of client, client will be stopped.
int pomelo_protocol_peer_disconnect(pomelo_protocol_peer_t * peer);

/// @brief Get round trip time of peer
int pomelo_protocol_peer_rtt(
    pomelo_protocol_peer_t * peer,
    uint64_t * mean,
    uint64_t * variance
);

/* -------------------------------------------------------------------------- */
/*                          External linkage APIs                             */
/* -------------------------------------------------------------------------- */

/// @brief The callback when a peer has connected to this socket
/// @param socket The socket
/// @param peer The connected peer
void pomelo_protocol_socket_on_connected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
);


/// @brief The callback when a peer has disconnected/lost connection
/// @param socket The socket
/// @param peer The disconnected/lost connection peer
void pomelo_protocol_socket_on_disconnected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
);


/// @brief The callback when socket has received a payload from peer.
/// @param socket The socket
/// @param peer The sender
/// @param packet The received packet
void pomelo_protocol_socket_on_received(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
);


/// @brief The callback when socket has completely stopped.
/// @param socket The socket
void pomelo_protocol_socket_on_stopped(pomelo_protocol_socket_t * socket);


/// @brief The callback with the result of connection request.
void pomelo_protocol_socket_on_connect_result(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_connect_result result
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_SRC_H
