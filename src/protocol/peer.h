#ifndef POMELO_PROTOCOL_PEER_SRC_H
#define POMELO_PROTOCOL_PEER_SRC_H
#include "protocol.h"
#include "utils/list.h"
#include "base/rtt.h"
#include "codec/packet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POMELO_REPlAY_PROTECTED_BUFFER_SIZE 256


/// @brief Replay protected structure
struct pomelo_protocol_replay_protector_s {
    /// @brief The buffer array
    uint64_t received_sequence[POMELO_REPlAY_PROTECTED_BUFFER_SIZE];

    /// @brief The most recent sequence received
    uint64_t most_recent_sequence;
};

typedef struct pomelo_protocol_replay_protector_s
    pomelo_protocol_replay_protector_t;


/// @brief The socket state of client
typedef enum pomelo_protocol_peer_client_state {
    POMELO_CLIENT_DISCONNECTING                 = -7,
    POMELO_CLIENT_CONNECT_TOKEN_EXPIRE          = -6,
    POMELO_CLIENT_INVALID_CONNECT_TOKEN         = -5,
    POMELO_CLIENT_CONNECTION_TIMED_OUT          = -4,
    POMELO_CLIENT_CONNECTION_RESPONSE_TIMED_OUT = -3,
    POMELO_CLIENT_CONNECTION_REQUEST_TIMED_OUT  = -2,
    POMELO_CLIENT_CONNECTION_DENIED             = -1,
    POMELO_CLIENT_DISCONNECTED                  =  0,
    POMELO_CLIENT_SENDING_CONNECTION_REQUEST    =  1,
    POMELO_CLIENT_SENDING_CONNECTION_RESPONSE   =  2,
    POMELO_CLIENT_CONNECTED                     =  3
} pomelo_protocol_peer_client_state;


/// @brief The socket state of a peer in server
typedef enum pomelo_protocol_peer_server_state {
    POMELO_SERVER_DISCONNECTING = -1,
    POMELO_SERVER_DISCONNECTED = 0,
    POMELO_SERVER_ANONYMOUS    = 1,
    POMELO_SERVER_UNCONFIRMED  = 2,
    POMELO_SERVER_CONNECTED    = 3
} pomelo_protocol_peer_server_state;


struct pomelo_protocol_peer_s {
    /// @brief The associated data with this peer
    void * extra_data;

    /// @brief The unique ID of peer
    int64_t client_id;

    /// @brief The client index of peer
    int client_index;

    /// The target address of peer
    pomelo_address_t address;

    /// The host socket
    pomelo_protocol_socket_t * socket;

    union {
        pomelo_protocol_peer_client_state client;
        pomelo_protocol_peer_server_state server;
    } state;

    /// @brief The last receive packet time (in nanoseconds)
    uint64_t last_recv_time;

    /// @brief Last time receive ping packet
    uint64_t last_recv_ping_time;

    /// @brief The timeout of peer (in nanoseconds)
    /// This is set by request connect token
    uint64_t timeout_ns;

    /// @brief The sequence number for sending packets
    uint64_t sequence_number;

    /// @brief Replay protection
    pomelo_protocol_replay_protector_t replay_protector;

    /// @brief The codec context for encoding & decoding
    pomelo_codec_packet_context_t codec_ctx;

    /// @brief Decoding key
    uint8_t decode_key[POMELO_KEY_BYTES];

    /// @brief Encoding key
    uint8_t encode_key[POMELO_KEY_BYTES];

    /// @brief The created time (in nanoseconds)
    uint64_t created_time_ns;

    /// @brief The allocator for peer
    pomelo_allocator_t * allocator;

    /// @brief Round trip time
    pomelo_rtt_calculator_t rtt;

    /// @brief Processing sending passes
    pomelo_list_t * send_passes;

    /// @brief Processing receiving passes
    pomelo_list_t * recv_passes;

    /* Specific for server */

    /// @brief The node in connected peer list.
    /// This property helps removing peer from list faster
    pomelo_list_node_t * connected_node;

    /// @brief The node in disconnecting peer list.
    /// This property helps removing peer from list faster
    pomelo_list_node_t * disconnecting_node;

    /// @brief The remain redundant disconnecting times to send
    int remain_redundant_disconnect;
};

/// @brief Initialize the peer
int pomelo_protocol_peer_init(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset the peer. This will keep the channels & transporter
void pomelo_protocol_peer_reset(pomelo_protocol_peer_t * peer);

/// @brief Cleanup the peer
void pomelo_protocol_peer_cleanup(pomelo_protocol_peer_t * peer);

/// @brief Protect replay packet for server socket
/// @return Returns 1 if the sequence number passes replay protection or 0 if
/// it does not pass.
int pomelo_protocol_peer_protect_replay(
    pomelo_protocol_peer_t * peer,
    uint64_t sequence_number
);

/// @brief Next ping sequence number of peer
uint64_t pomelo_protocol_peer_next_ping_sequence(pomelo_protocol_peer_t * peer);

/// @brief Receive ping sequence
void pomelo_protocol_peer_receive_pong(
    pomelo_protocol_peer_t * peer,
    pomelo_packet_pong_t * packet,
    uint64_t recv_time
);

/// @brief Terminate all sending & receiving passes of peer
void pomelo_protocol_peer_terminate_passes(pomelo_protocol_peer_t * peer);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_PEER_SRC_H
