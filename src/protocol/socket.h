#ifndef POMELO_PROTOCOL_SOCKET_SRC_H
#define POMELO_PROTOCOL_SOCKET_SRC_H
#include "platform/platform.h"
#include "base/buffer.h"
#include "adapter/adapter.h"
#include "pass.h"
#include "clock.h"


/// Default max number of clients for server
#define POMELO_DEFAULT_MAX_CLIENTS 32

/// The ping packet sending frequency when socket is connected
#define POMELO_PING_FREQUENCY_HZ 10 // Hz

/// The pong reply packet frequency
#define POMELO_PONG_REPLY_FREQUENCY_HZ 20 // Hz

/// The sending connection request & response frequency
#define POMELO_CONNECTION_REQUEST_RESPONSE_FREQUENCY_HZ 10 // Hz

/// The disconnect packet sending frequency
#define POMELO_DISCONNECT_FREQUENCY_HZ 10

/// The disconnect packet sending limit
#define POMELO_DISCONNECT_REDUNDANT_LIMIT 10


#ifdef __cplusplus
extern "C" {
#endif


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

    /// @brief The socket has been stopping
    POMELO_PROTOCOL_SOCKET_STATE_STOPPING,

    /// @brief The socket is running
    POMELO_PROTOCOL_SOCKET_STATE_RUNNING
} pomelo_protocol_socket_state;

/// @brief The collection of pools of socket
typedef struct pomelo_protocol_socket_pools_s pomelo_protocol_socket_pools_t;

/// @brief The socket creating options
typedef struct pomelo_protocol_socket_options_s
    pomelo_protocol_socket_options_t;

/// @brief The statistic information of socket
typedef struct pomelo_protocol_socket_statistic_s
    pomelo_protocol_socket_statistic_t;


struct pomelo_protocol_socket_options_s {
    /// @brief The allocator to be used in system.
    /// If NULL is set, socket will use the default allocator
    pomelo_allocator_t * allocator;

    /// @brief The platform which this socket relies on
    pomelo_platform_t * platform;

    /// @brief The protocol context
    pomelo_protocol_context_t * context;
};


struct pomelo_protocol_socket_pools_s {
    /// @brief The packet pools
    pomelo_pool_t * packets[POMELO_PACKET_TYPE_COUNT];

    /// @brief The pool for decode packet info objects
    pomelo_pool_t * recv_pass;

    /// @brief The sending passes pool
    pomelo_pool_t * send_pass;
};


struct pomelo_protocol_socket_statistic_s {
    /// @brief The number of bytes of all valid packets
    uint64_t valid_recv_bytes;

    /// @brief The number of bytes of all invalid packets
    uint64_t invalid_recv_bytes;
};


struct pomelo_protocol_socket_s {
    /// @brief The extra data for socket
    void * extra_data;

    /// @brief The socket platform
    pomelo_platform_t * platform;

    /// @brief Adapter of socket
    pomelo_adapter_t * adapter;

    /// @brief The work group of socket
    pomelo_platform_task_group_t * task_group;

    /// @brief The socket allocator
    pomelo_allocator_t * allocator;

    /// @brief The socket running mode
    pomelo_protocol_socket_mode mode;

    /// @brief Collection of pools
    pomelo_protocol_socket_pools_t pools;

    /// @brief The protocol context
    pomelo_protocol_context_t * context;

    /// @brief The running state
    pomelo_protocol_socket_state state;

    /// @brief The statistic of socket
    pomelo_protocol_socket_statistic_t statistic;

    /// @brief Clock offset of socket
    pomelo_protocol_clock_t clock;

    /// @brief If this flag is set, outgoing packets will no be encrypted
    bool no_encrypt;
};


/* -------------------------------------------------------------------------- */
/*                               Init & Cleanup                               */
/* -------------------------------------------------------------------------- */

/// Initialize all structures of socket generally
int pomelo_protocol_socket_init(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_socket_options_t * options
);

/// Destroy all pools of socket
void pomelo_protocol_socket_cleanup(pomelo_protocol_socket_t * socket);


/* -------------------------------------------------------------------------- */
/*                            Platform callbacks                              */
/* -------------------------------------------------------------------------- */

/// @brief Payload allocation callback
void pomelo_protocol_socket_on_alloc(
    pomelo_protocol_socket_t * socket,
    pomelo_buffer_vector_t * buffer_vector
);

/// @brief The receiving callback
void pomelo_protocol_socket_on_recv(
    pomelo_protocol_socket_t * socket,
    pomelo_address_t * address,
    pomelo_buffer_vector_t * buffer_vector,
    int status,
    bool encrypted
);

/// @brief The sending callback
void pomelo_protocol_socket_on_sent(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_send_pass_t * pass,
    int status
);


/* -------------------------------------------------------------------------- */
/*                             Incoming packets                               */
/* -------------------------------------------------------------------------- */

/// @brief process after receiving a packet
void pomelo_protocol_socket_recv_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_t * packet,
    uint64_t recv_time,
    int result
);

/// @brief Process after receiving a request packet
void pomelo_protocol_socket_recv_request_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
);

/// @brief Process after receiving a denied packet
void pomelo_protocol_socket_recv_denied_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
);

/// @brief Process after receiving a challenge packet
void pomelo_protocol_socket_recv_challenge_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
);

/// @brief Process after receiving a response packet
void pomelo_protocol_socket_recv_response_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
);

/// @brief Process after receiving a ping packet
void pomelo_protocol_socket_recv_ping_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    uint64_t recv_time,
    int result
);

/// @brief Process after receiving a payload packet
void pomelo_protocol_socket_recv_payload_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_payload_t * packet,
    int result
);

/// @brief Process after receiving a disconnect packet
void pomelo_protocol_socket_recv_disconnect_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
);

/// @brief Process after receiving a pong packet
void pomelo_protocol_socket_recv_pong_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_pong_t * packet,
    uint64_t recv_time,
    int result
);


/* -------------------------------------------------------------------------- */
/*                             Outgoing packets                               */
/* -------------------------------------------------------------------------- */

/// @brief Process after sending request packet
void pomelo_protocol_socket_sent_request_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
);

/// @brief Process after sending a packet
void pomelo_protocol_socket_sent_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_t * packet,
    int result
);

/// @brief Process after sending a request packet
void pomelo_protocol_socket_send_request_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
);

/// @brief Process after sending a denied packet
void pomelo_protocol_socket_sent_denied_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
);

/// @brief Process after sending a challenge packet
void pomelo_protocol_socket_sent_challenge_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
);

/// @brief Process after sending a response packet
void pomelo_protocol_socket_sent_response_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
);

/// @brief Process after sending ping packet
void pomelo_protocol_socket_sent_ping_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    int result
);

/// @brief Process after sending a payload packet
void pomelo_protocol_socket_sent_payload_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_payload_t * packet,
    int result
);

/// @brief Process after sending disconnect packet
void pomelo_protocol_socket_sent_disconnect_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
);

/// @brief Process after sending a pong packet
void pomelo_protocol_socket_sent_pong_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_pong_t * packet,
    int result
);

/* -------------------------------------------------------------------------- */
/*                             Common functions                               */
/* -------------------------------------------------------------------------- */

/// @brief Prepare a send pass with associaited packet & payload
pomelo_protocol_send_pass_t * pomelo_protocol_socket_prepare_send_pass(
    pomelo_protocol_socket_t * socket,
    pomelo_packet_type type
);

/// @brief Cancel sending pass
void pomelo_protocol_socket_cancel_send_pass(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_send_pass_t * pass
);

/// @brief Release the received packet
void pomelo_protocol_socket_release_packet(
    pomelo_protocol_socket_t * socket,
    pomelo_packet_t * packet
);

/* -------------------------------------------------------------------------- */
/*                            Specific functions                              */
/* -------------------------------------------------------------------------- */

/// @brief This callback is called after canceling all working jobs
void pomelo_protocol_socket_stop_deferred(
    pomelo_protocol_socket_t * socket
);

/// @brief Reply a pong packet
void pomelo_protocol_socket_reply_pong(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    uint64_t ping_sequence,
    uint64_t ping_recv_time
);


/// @brief Send payload
/// TOOD: Change to send buffer instead
/// @param socket The socket
/// @param peer The target peer
/// @param buffer The buffer to send
/// @param length Length of buffer to send
int pomelo_protocol_socket_send(
    pomelo_protocol_socket_t * socket,
    pomelo_protocol_peer_t * peer,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
);


/// @brief Sync socket time
void pomelo_protocol_socket_sync_time(
    pomelo_protocol_socket_t * socket,
    uint64_t req_send_time, // t0
    uint64_t req_recv_time, // t1
    uint64_t res_delta_time, // t2
    uint64_t res_recv_time  // t3
);


/* -------------------------------------------------------------------------- */
/*                         Packet callback functions                          */
/* -------------------------------------------------------------------------- */

/// @brief Init packet request
int pomelo_socket_packet_request_init(
    pomelo_packet_request_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet request
int pomelo_socket_packet_request_reset(
    pomelo_packet_request_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Init packet response
int pomelo_socket_packet_response_init(
    pomelo_packet_response_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet response
int pomelo_socket_packet_response_reset(
    pomelo_packet_response_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Init packet denied
int pomelo_socket_packet_denied_init(
    pomelo_packet_denied_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet denied
int pomelo_socket_packet_denied_reset(
    pomelo_packet_denied_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Init packet challenge
int pomelo_socket_packet_challenge_init(
    pomelo_packet_challenge_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet challenge
int pomelo_socket_packet_challenge_reset(
    pomelo_packet_challenge_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Init packet payload
int pomelo_socket_packet_payload_init(
    pomelo_packet_payload_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet payload
int pomelo_socket_packet_payload_reset(
    pomelo_packet_payload_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Init packet ping
int pomelo_socket_packet_ping_init(
    pomelo_packet_ping_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet ping
int pomelo_socket_packet_ping_reset(
    pomelo_packet_ping_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Init packet disconnect
int pomelo_socket_packet_disconnect_init(
    pomelo_packet_disconnect_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet disconnect
int pomelo_socket_packet_disconnect_reset(
    pomelo_packet_disconnect_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Init packet pong
int pomelo_socket_packet_pong_init(
    pomelo_packet_pong_t * packet,
    pomelo_protocol_socket_t * socket
);

/// @brief Reset packet pong
int pomelo_socket_packet_pong_reset(
    pomelo_packet_pong_t * packet,
    pomelo_protocol_socket_t * socket
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_SOCKET_SRC_H
