#ifndef POMELO_PROTOCOL_CLIENT_SRC_H
#define POMELO_PROTOCOL_CLIENT_SRC_H
#include <stdbool.h>
#include "protocol.h"
#include "platform/platform.h"
#include "emitter.h"
#include "socket.h"


#ifdef __cplusplus
extern "C" {
#endif


/// @brief Stopped reason
typedef enum pomelo_protocol_client_stopped_reason {
    POMELO_SOCKET_CLIENT_STOPPED_UNSET,
    POMELO_SOCKET_CLIENT_STOPPED_LOST_CONNECTION,
    POMELO_SOCKET_CLIENT_STOPPED_DENIED,
    POMELO_SOCKET_CLIENT_STOPPED_DISCONNECTED,
    POMELO_SOCKET_CLIENT_STOPPED_REQUEST_TIMED_OUT,
    POMELO_SOCKET_CLIENT_STOPPED_ERROR
} pomelo_protocol_client_stopped_reason;


/// @brief The collection of client emitters
typedef struct pomelo_protocol_client_emitters_s
    pomelo_protocol_client_emitters_t;

struct pomelo_protocol_client_emitters_s {
    /// @brief The request packet emitter
    pomelo_protocol_emitter_t request;

    /// @brief The response packet emitter
    pomelo_protocol_emitter_t response;

    /// @brief The ping emitter
    pomelo_protocol_emitter_t ping;

    /// @brief The disconnect emitter
    pomelo_protocol_emitter_t disconnect;
};


struct pomelo_protocol_client_s {
    /// @brief The base socket
    pomelo_protocol_socket_t socket;

    /// @brief The peer of client
    pomelo_protocol_peer_t * peer;

    /// @brief The connect token
    pomelo_connect_token_t connect_token;

    /// @brief The raw data of connect token
    uint8_t connect_token_raw[POMELO_CONNECT_TOKEN_BYTES];

    /// @brief The collection of emitters
    pomelo_protocol_client_emitters_t emitters;

    /// @brief The index of address (in connect token) which this client is
    /// connecting to
    int address_index;

    /// @brief Stopped reason of socket client
    pomelo_protocol_client_stopped_reason stopped_reason;
};


/// @brief Destroy the client
void pomelo_protocol_client_destroy(pomelo_protocol_client_t * client);

/// @brief The socket client starts connecting to server
/// @return 0 on success, or an error code < 0 on failure
int pomelo_protocol_client_start(pomelo_protocol_client_t * client);

/// @brief Stop the client
void pomelo_protocol_client_stop(pomelo_protocol_client_t * client);

/// @brief Deferred stopping client
void pomelo_protocol_client_stop_deferred(pomelo_protocol_client_t * client);

/* -------------------------------------------------------------------------- */
/*                            Receiving callbacks                             */
/* -------------------------------------------------------------------------- */

/// @brief Receiving callback for socket client
int pomelo_protocol_client_on_recv(
    pomelo_protocol_client_t * client,
    pomelo_address_t * address,
    pomelo_buffer_t * buffer,
    size_t length,
    uint64_t recv_time,
    bool encrypted
);

/* -------------------------------------------------------------------------- */
/*                         Client incoming packets                            */
/* -------------------------------------------------------------------------- */

/// @brief Process denied packet
void pomelo_protocol_client_recv_denied_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
);

/// @brief Process challenge packet
void pomelo_protocol_client_recv_challenge_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
);

/// @brief Process disconnect packet
void pomelo_protocol_client_recv_disconnect_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
);

/// @brief Process ping packet
void pomelo_protocol_client_recv_ping_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    uint64_t recv_time,
    int result
);

/// @brief Handle ping packet when sending response
void pomelo_protocol_client_handle_ping_on_response(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    int64_t client_id
);


/// @brief Change state to connected after receiving ping
int pomelo_protocol_client_update_state_connected(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    int64_t client_id
);

/* -------------------------------------------------------------------------- */
/*                         Client specific functions                          */
/* -------------------------------------------------------------------------- */

/// @brief Connect to specific address
int pomelo_protocol_client_connect_address(
    pomelo_protocol_client_t * client,
    pomelo_address_t * address
);

/// @brief Start sending request packets repeatly
/// @return Returns 0 on success, or an error code < 0 on failure
int pomelo_protocol_client_request_start(pomelo_protocol_client_t * client);

/// @brief Stop sending request packets repeatly
/// @return Returns 0 on success, or an error code < 0 on failure
int pomelo_protocol_client_request_stop(pomelo_protocol_client_t * client);

/// @brief Start sending response packets repeatly
int pomelo_protocol_client_response_start(pomelo_protocol_client_t * client);

/// @brief Stop sending response packets repeatly
int pomelo_protocol_client_response_stop(pomelo_protocol_client_t * client);

/// @brief Start sending ping packets repeatly
int pomelo_protocol_client_ping_start(pomelo_protocol_client_t * client);

/// @brief Process before sending ping
void pomelo_protocol_client_ping_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
);

/// @brief Process before sending response
void pomelo_protocol_client_response_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
);

/// @brief Stop sending ping packets repeatly
int pomelo_protocol_client_ping_stop(pomelo_protocol_client_t * client);

/// @brief Start sending disconnect packets repeatly
int pomelo_protocol_client_disconnect_start(pomelo_protocol_client_t * client);

/// @brief Trigger callback for sending disconnect
void pomelo_protocol_client_disconnect_trigger(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
);

/// @brief The done callback when client sends enough disconnect packets.
void pomelo_protocol_client_disconnect_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
);

/// @brief Stop sending disconnect packets repeatly
int pomelo_protocol_client_disconnect_stop(pomelo_protocol_client_t * client);

/// @brief Process disconnected peer for client socket
void pomelo_protocol_client_process_disconnected_peer(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer
);

/// @brief Get the next connecting address of client
/// @return Next address or NULL if there is no more
pomelo_address_t * pomelo_protocol_client_next_connect_address(
    pomelo_protocol_client_t * client
);

/// @brief Process when request timed out
void pomelo_protocol_client_request_timed_out_callback(
    pomelo_protocol_client_t * client,
    pomelo_protocol_emitter_t * emitter
);

/// @brief Disconnect a connected peer
int pomelo_protocol_client_disconnect_peer(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer
);


/* -------------------------------------------------------------------------- */
/*                          Client outgoing packets                           */
/* -------------------------------------------------------------------------- */


/// @brief Packet request sending callback
void pomelo_protocol_client_sent_request_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
);

/// @brief Packet response sending callback
void pomelo_protocol_client_sent_response_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
);

/// @brief Packet ping sending callback
void pomelo_protocol_client_sent_ping_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    int result
);

/// @brief Packet disconnect sending callback
void pomelo_protocol_client_sent_disconnect_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_CLIENT_SRC_H
