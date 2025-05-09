#ifndef POMELO_PROTOCOL_SOCKET_SRC_H
#define POMELO_PROTOCOL_SOCKET_SRC_H
#include "platform/platform.h"
#include "base/buffer.h"
#include "sender.h"
#include "receiver.h"
#ifdef __cplusplus
extern "C" {
#endif

/// Default max number of clients for server
#define POMELO_DEFAULT_MAX_CLIENTS 32

/// The keep alive packet sending frequency
#define POMELO_KEEP_ALIVE_FREQUENCY_HZ 10 // Hz

/// The sending connection request & response frequency
#define POMELO_CONNECTION_REQUEST_RESPONSE_FREQUENCY_HZ 10 // Hz

/// The disconnect packet sending frequency
#define POMELO_DISCONNECT_FREQUENCY_HZ 10

/// The anonymous peer removal frequency
#define POMELO_ANONYMOUS_REMOVAL_FREQUENCY_HZ 1 // Hz

/// The disconnect packet sending limit
#define POMELO_DISCONNECT_REDUNDANT_LIMIT 10

/// The flag of no encrypt
#define POMELO_PROTOCOL_SOCKET_FLAG_NO_ENCRYPT (1 << 0)


/// @brief Socket mode
typedef enum pomelo_protocol_socket_mode {
    /// @brief The socket is running as server socket (listening).
    POMELO_PROTOCOL_SOCKET_MODE_SERVER,

    /// @brief The socket is running as client socket (connecting).
    POMELO_PROTOCOL_SOCKET_MODE_CLIENT,
} pomelo_protocol_socket_mode;


/// @brief Socket state
typedef enum pomelo_protocol_socket_state {
    /// @brief The socket has completely stopped
    POMELO_PROTOCOL_SOCKET_STATE_STOPPED,

    /// @brief The socket is stopping
    POMELO_PROTOCOL_SOCKET_STATE_STOPPING,

    /// @brief The socket is running
    POMELO_PROTOCOL_SOCKET_STATE_RUNNING
} pomelo_protocol_socket_state;


/// @brief The socket creating options
typedef struct pomelo_protocol_socket_options_s
    pomelo_protocol_socket_options_t;


/// @brief The packet validation result
typedef struct pomelo_protocol_packet_validation_s
    pomelo_protocol_packet_validation_t;


/// @brief The packet incoming information
typedef struct pomelo_protocol_packet_incoming_s
    pomelo_protocol_packet_incoming_t;


struct pomelo_protocol_packet_validation_s {
    /// @brief The peer
    pomelo_protocol_peer_t * peer;

    /// @brief The packet header
    pomelo_protocol_packet_header_t header;
};


struct pomelo_protocol_packet_incoming_s {
    /// @brief The address of incoming packet
    pomelo_address_t * address;

    /// @brief The view of incoming packet
    pomelo_buffer_view_t * view;

    /// @brief Whether the packet is encrypted
    bool encrypted;
};


struct pomelo_protocol_socket_options_s {
    /// @brief The platform which this socket relies on
    pomelo_platform_t * platform;

    /// @brief The adapter of socket
    pomelo_adapter_t * adapter;

    /// @brief The sequencer of socket
    pomelo_sequencer_t * sequencer;

    /// @brief The socket running mode
    pomelo_protocol_socket_mode mode;

    /// @brief The flags of socket
    uint32_t flags;
};


struct pomelo_protocol_socket_s {
    /// @brief The protocol context
    pomelo_protocol_context_t * context;

    /// @brief The extra data for socket
    void * extra;

    /// @brief The socket platform
    pomelo_platform_t * platform;

    /// @brief Adapter of socket
    pomelo_adapter_t * adapter;

    /// @brief The socket running mode
    pomelo_protocol_socket_mode mode;

    /// @brief The running state
    pomelo_protocol_socket_state state;

    /// @brief The statistic of socket
    pomelo_protocol_socket_statistic_t statistic;

    /// @brief Flags of socket
    uint32_t flags;

    /// @brief The sequencer of socket
    pomelo_sequencer_t * sequencer;

    /// @brief The stop task of socket
    pomelo_sequencer_task_t stop_task;

    /// @brief The destroy task of socket
    pomelo_sequencer_task_t destroy_task;
};


/* -------------------------------------------------------------------------- */
/*                               Init & Cleanup                               */
/* -------------------------------------------------------------------------- */

/// @brief The callback on socket allocated
int pomelo_protocol_socket_on_alloc(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_context_t * context
);


/// @brief The callback on socket freed
void pomelo_protocol_socket_on_free(pomelo_protocol_socket_t * socket);


/// Initialize all structures of socket generally
int pomelo_protocol_socket_init(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_socket_options_t * options
);

/// Destroy all pools of socket
void pomelo_protocol_socket_cleanup(pomelo_protocol_socket_t * socket);


/// @brief Stop the socket
void pomelo_protocol_socket_stop_deferred(pomelo_protocol_socket_t * socket);


/// @brief Destroy the socket
void pomelo_protocol_socket_destroy_deferred(pomelo_protocol_socket_t * socket);


/* -------------------------------------------------------------------------- */
/*                             Incoming packets                               */
/* -------------------------------------------------------------------------- */

/// @brief Process after receiving a packet
void pomelo_protocol_socket_recv_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
);


/// @brief Process after receiving a packet failed
void pomelo_protocol_socket_recv_failed(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_header_t * header
);


/// @brief Process after receiving a payload packet
void pomelo_protocol_socket_recv_payload(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_payload_t * packet
);


/// @brief Process after receiving a disconnect packet
void pomelo_protocol_socket_recv_disconnect(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_disconnect_t * packet
);


/* -------------------------------------------------------------------------- */
/*                             Outgoing packets                               */
/* -------------------------------------------------------------------------- */

/// @brief Process after sending a packet
void pomelo_protocol_socket_sent_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
);


/* -------------------------------------------------------------------------- */
/*                            Specific functions                              */
/* -------------------------------------------------------------------------- */

/// @brief Send payload
/// @param socket The socket
/// @param peer The target peer
/// @param views The views of buffer
/// @param nviews The number of views
int pomelo_protocol_socket_send(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_view_t * views,
    size_t nviews
);


/// @brief Disconnect a peer
void pomelo_protocol_socket_disconnect_peer(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
);


/// @brief Process after sender is completed
void pomelo_protocol_socket_handle_sender_complete(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_sender_t * sender
);


/// @brief Process after receiver is completed
void pomelo_protocol_socket_handle_receiver_complete(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_receiver_t * receiver
);


/// @brief Accept an incoming packet
void pomelo_protocol_socket_accept(
    pomelo_protocol_socket_t * socket,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
);


/// @brief Validate the incoming data before decoding
/// @return 0 on success, or an error code < 0 on failure
int pomelo_protocol_socket_validate_incoming(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_packet_incoming_t * incoming,
    pomelo_protocol_packet_validation_t * validation
);


/// @brief Validate the incoming packet after decoding
/// @return 0 on success, or an error code < 0 on failure
int pomelo_protocol_socket_validate_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
);


/// @brief Validate the keep alive packet
/// @return 0 on success, or an error code < 0 on failure
int pomelo_protocol_socket_validate_keep_alive(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_keep_alive_t * packet
);


/// @brief Dispatch an outgoing packet
void pomelo_protocol_socket_dispatch(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
);


/// @brief Dispatch disconnected event
void pomelo_protocol_socket_dispatch_peer_disconnected(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_SOCKET_SRC_H
