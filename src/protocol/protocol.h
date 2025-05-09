#ifndef POMELO_PROTOCOL_SRC_H
#define POMELO_PROTOCOL_SRC_H
#include "pomelo/allocator.h"
#include "pomelo/address.h"
#include "pomelo/statistic/statistic-protocol.h"
#include "platform/platform.h"
#include "adapter/adapter.h"
#include "base/buffer.h"
#include "base/sequencer.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Connect result
typedef enum pomelo_protocol_connect_result_e {
    /// @brief The connection has timed out
    POMELO_PROTOCOL_SOCKET_CONNECT_TIMED_OUT = -2,

    /// @brief The connection has been denied
    POMELO_PROTOCOL_SOCKET_CONNECT_DENIED = -1,

    /// @brief The connection has been successfully established
    POMELO_PROTOCOL_SOCKET_CONNECT_SUCCESS = 0
} pomelo_protocol_connect_result;


/// @brief The protocol context
typedef struct pomelo_protocol_context_s pomelo_protocol_context_t;

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
typedef struct pomelo_protocol_context_options_s
    pomelo_protocol_context_options_t;

/// @brief The socket server creating options
typedef struct pomelo_protocol_server_options_s
    pomelo_protocol_server_options_t;

/// @brief The socket client creating options
typedef struct pomelo_protocol_client_options_s
    pomelo_protocol_client_options_t;

/// @brief The sender
typedef struct pomelo_protocol_sender_s pomelo_protocol_sender_t;

/// @brief The receiver
typedef struct pomelo_protocol_receiver_s pomelo_protocol_receiver_t;

/// The crypto context
typedef struct pomelo_protocol_crypto_context_s
    pomelo_protocol_crypto_context_t;

/// @brief The packet header
typedef struct pomelo_protocol_packet_header_s pomelo_protocol_packet_header_t;

/// @brief The statistic information of socket
typedef struct pomelo_protocol_socket_statistic_s
    pomelo_protocol_socket_statistic_t;


struct pomelo_protocol_context_options_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief The buffer context
    pomelo_buffer_context_t * buffer_context;

    /// @brief Capacity of payload
    size_t payload_capacity;
};


struct pomelo_protocol_server_options_s {
    /// @brief The platform which this socket relies on
    pomelo_platform_t * platform;

    /// @brief The adapter
    pomelo_adapter_t * adapter;

    /// @brief The protocol context
    pomelo_protocol_context_t * context;

    /// @brief The sequencer of client
    pomelo_sequencer_t * sequencer;

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
    /// @brief The platform which this socket relies on
    pomelo_platform_t * platform;

    /// @brief The adapter
    pomelo_adapter_t * adapter;

    /// @brief The protocol context
    pomelo_protocol_context_t * context;

    /// @brief The sequencer of client
    pomelo_sequencer_t * sequencer;

    /// @brief The connect token
    uint8_t * connect_token;
};


struct pomelo_protocol_socket_statistic_s {
    /// @brief The number of bytes of all valid packets
    uint64_t valid_recv_bytes;

    /// @brief The number of bytes of all invalid packets
    uint64_t invalid_recv_bytes;
};


/* -------------------------------------------------------------------------- */
/*                               Context APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Create new protocol context
/// @param options The creating options
/// @return New protocol context or NULl on failure
pomelo_protocol_context_t * pomelo_protocol_context_create(
    pomelo_protocol_context_options_t * options
);


/// @brief Destroy aprotocol context
/// @param context The context to destroy
void pomelo_protocol_context_destroy(pomelo_protocol_context_t * context);


/// @brief Get the statistic of protocol context
void pomelo_protocol_context_statistic(
    pomelo_protocol_context_t * context,
    pomelo_statistic_protocol_t * statistic
);


/* -------------------------------------------------------------------------- */
/*                                Socket APIs                                 */
/* -------------------------------------------------------------------------- */


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
/// @param socket The socket
/// @return 0 on success, or an error code < 0 on failure. 
void pomelo_protocol_socket_stop(pomelo_protocol_socket_t * socket);


/// @brief Get the socket extra data
void * pomelo_protocol_socket_get_extra(pomelo_protocol_socket_t * socket);


/// @brief Set the socket data for socket
void pomelo_protocol_socket_set_extra(
    pomelo_protocol_socket_t * socket,
    void * data
);


/// @brief Get the statistic of socket
pomelo_protocol_socket_statistic_t * pomelo_protocol_socket_statistic(
    pomelo_protocol_socket_t * socket
);


/* -------------------------------------------------------------------------- */
/*                              Socket peer APIs                              */
/* -------------------------------------------------------------------------- */


/// @brief Get the client ID of peer
int64_t pomelo_protocol_peer_get_client_id(pomelo_protocol_peer_t * peer);


/// @brief Get the peer address
pomelo_address_t * pomelo_protocol_peer_get_address(
    pomelo_protocol_peer_t * peer
);


/// @brief Send a payload.
/// @param peer The target peer
/// @param views The views of buffer
/// @param nviews The number of views
int pomelo_protocol_peer_send(
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_view_t * views,
    size_t nviews
);


/// @brief Get the peer extra data
void * pomelo_protocol_peer_get_extra(pomelo_protocol_peer_t * peer);


/// @brief Set the extra data for peer
void pomelo_protocol_peer_set_extra(pomelo_protocol_peer_t * peer, void * data);


/// @brief Disconnect a peer.
/// If this is the peer of client, client will be stopped.
int pomelo_protocol_peer_disconnect(pomelo_protocol_peer_t * peer);


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
    pomelo_buffer_view_t * view
);


/// @brief The callback with the result of connection request.
void pomelo_protocol_socket_on_connect_result(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_connect_result result
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_SRC_H
