#ifndef POMELO_PROTOCOL_PEER_SRC_H
#define POMELO_PROTOCOL_PEER_SRC_H
#include "pomelo/constants.h"
#include "utils/list.h"
#include "protocol.h"
#include "packet.h"
#ifdef __cplusplus
extern "C" {
#endif


#define POMELO_REPLAY_PROTECTED_BUFFER_SIZE 256

/// @brief Peer is confirmed
#define POMELO_PEER_FLAG_CONFIRMED           (1 << 0)

/// @brief Peer is processing response packet
#define POMELO_PEER_FLAG_PROCESSING_RESPONSE (1 << 1)


/// @brief Replay protected structure
typedef struct pomelo_protocol_replay_protector_s
    pomelo_protocol_replay_protector_t;


/// @brief The info for peer
typedef struct pomelo_protocol_peer_info_s pomelo_protocol_peer_info_t;


struct pomelo_protocol_replay_protector_s {
    /// @brief The buffer array
    uint64_t received_sequence[POMELO_REPLAY_PROTECTED_BUFFER_SIZE];

    /// @brief The most recent sequence received
    uint64_t most_recent_sequence;
};


struct pomelo_protocol_peer_info_s {
    /// @brief The socket
    pomelo_protocol_socket_t * socket;

    /// @brief The created time (in nanoseconds)
    uint64_t created_time_ns;
};


/// @brief The state of a peer
typedef enum pomelo_protocol_peer_state {
    POMELO_PROTOCOL_PEER_DISCONNECTING          = -7,
    POMELO_PROTOCOL_PEER_CONNECT_TOKEN_EXPIRE   = -6,
    POMELO_PROTOCOL_PEER_INVALID_CONNECT_TOKEN  = -5,
    POMELO_PROTOCOL_PEER_TIMED_OUT              = -4,
    POMELO_PROTOCOL_PEER_RESPONSE_TIMED_OUT     = -3,
    POMELO_PROTOCOL_PEER_REQUEST_TIMED_OUT      = -2,
    POMELO_PROTOCOL_PEER_DENIED                 = -1,
    POMELO_PROTOCOL_PEER_DISCONNECTED           =  0,
    POMELO_PROTOCOL_PEER_REQUEST                =  1,
    POMELO_PROTOCOL_PEER_RESPONSE               =  2,
    POMELO_PROTOCOL_PEER_CHALLENGE              =  3,
    POMELO_PROTOCOL_PEER_CONNECTED              =  4
} pomelo_protocol_peer_state;


struct pomelo_protocol_peer_s {
    /// @brief The context
    pomelo_protocol_context_t * context;

    /// @brief The associated data with this peer
    void * extra;

    /// @brief The unique ID of peer
    int64_t client_id;

    /// The target address of peer
    pomelo_address_t address;

    /// The host socket
    pomelo_protocol_socket_t * socket;

    /// @brief The state of peer
    pomelo_protocol_peer_state state;

    /// @brief The last receive packet time (in nanoseconds)
    uint64_t last_recv_time;

    /// @brief Last time receive keep alive packet
    uint64_t last_recv_time_keep_alive;

    /// @brief The timeout of peer (in nanoseconds)
    /// This is set by request connect token
    uint64_t timeout_ns;

    /// @brief The sequence number for sending packets
    uint64_t sequence_number;

    /// @brief Replay protection
    pomelo_protocol_replay_protector_t replay_protector;

    /// @brief The codec context for encoding & decoding
    pomelo_protocol_crypto_context_t * crypto_ctx;

    /// @brief The created time (in nanoseconds)
    uint64_t created_time_ns;

    /// @brief Processing senders
    pomelo_list_t * senders;

    /// @brief Processing receivers
    pomelo_list_t * receivers;

    /* Specific for server */

    /// @brief The entry in connected / disconnecting / anonymous / denied list.
    pomelo_list_entry_t * entry;

    /// @brief Flags of peer
    uint32_t flags;

    /// @brief The remain redundant disconnecting times to send
    int remain_redundant_disconnect;

    /// @brief The user data
    uint8_t user_data[POMELO_USER_DATA_BYTES];

    /// @brief The disconnect task of peer
    pomelo_sequencer_task_t disconnect_task;
};


/// @brief Initialize callback for peer
int pomelo_protocol_peer_on_alloc(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_context_t * context
);


/// @brief Acquire callback for peer
int pomelo_protocol_peer_init(
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_peer_info_t * info
);


/// @brief Release callback for peer
void pomelo_protocol_peer_cleanup(pomelo_protocol_peer_t * peer);


/// @brief Cleanup callback for peer
void pomelo_protocol_peer_on_free(pomelo_protocol_peer_t * peer);


/// @brief Protect replay packet for server socket
/// @return 0 if the sequence number passes replay protection, -1 if it does
/// not pass.
int pomelo_protocol_peer_protect_replay(
    pomelo_protocol_peer_t * peer,
    uint64_t sequence_number
);


/// @brief Terminate all senders & receivers of peer
void pomelo_protocol_peer_cancel_senders_and_receivers(
    pomelo_protocol_peer_t * peer
);

/// @brief Next sequence number of peer
#define pomelo_protocol_peer_next_sequence(peer) ((peer)->sequence_number++)


/// @brief Disconnect peer deferred
void pomelo_protocol_peer_disconnect_deferred(pomelo_protocol_peer_t * peer);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_PEER_SRC_H
