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


struct pomelo_protocol_client_s {
    /// @brief The base socket
    pomelo_protocol_socket_t socket;

    /// @brief The peer of client
    pomelo_protocol_peer_t * peer;

    /// @brief The request packet emitter
    pomelo_protocol_emitter_t emitter_request;

    /// @brief The response packet emitter
    pomelo_protocol_emitter_t emitter_response;

    /// @brief The keep alive emitter
    pomelo_protocol_emitter_t emitter_keep_alive;

    /// @brief The disconnect emitter
    pomelo_protocol_emitter_t emitter_disconnect;

    /// @brief The index of address (in connect token) which this client is
    /// connecting to
    int address_index;

    /// @brief The connect token
    pomelo_connect_token_t connect_token;

    /// @brief The private part of connect token
    uint8_t connect_token_data[POMELO_CONNECT_TOKEN_BYTES];

    /// @brief The sequence of challenge token
    uint64_t challenge_token_sequence;

    /// @brief The encrypted challenge token
    uint8_t challenge_token_data[POMELO_CHALLENGE_TOKEN_BYTES];
};


/// @brief The callback on client allocated
int pomelo_protocol_client_on_alloc(
    pomelo_protocol_client_t * client,
    pomelo_protocol_context_t * context
);


/// @brief The callback on client freed
void pomelo_protocol_client_on_free(pomelo_protocol_client_t * client);


/// @brief Initialize the client
int pomelo_protocol_client_init(
    pomelo_protocol_client_t * client,
    pomelo_protocol_client_options_t * options
);


/// @brief Cleanup the client
void pomelo_protocol_client_cleanup(pomelo_protocol_client_t * client);


/// @brief Destroy the client
void pomelo_protocol_client_destroy(pomelo_protocol_client_t * client);


/// @brief The socket client starts connecting to server
int pomelo_protocol_client_start(pomelo_protocol_client_t * client);


/// @brief Stop the client
void pomelo_protocol_client_stop(pomelo_protocol_client_t * client);


/// @brief Validate the incoming packet
int pomelo_protocol_client_validate(
    pomelo_protocol_client_t * client,
    pomelo_protocol_packet_incoming_t * incoming,
    pomelo_protocol_packet_validation_t * validation
);

/* -------------------------------------------------------------------------- */
/*                         Client incoming packets                            */
/* -------------------------------------------------------------------------- */

/// @brief Process the incoming packet
void pomelo_protocol_client_recv_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
);


/// @brief Process the incoming packet failed
void pomelo_protocol_client_recv_failed(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_header_t * header
);


/// @brief Process denied packet
void pomelo_protocol_client_recv_denied(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_denied_t * packet
);


/// @brief Process challenge packet
void pomelo_protocol_client_recv_challenge(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_challenge_t * packet
);


/// @brief Process disconnect packet
void pomelo_protocol_client_recv_disconnect(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_disconnect_t * packet
);


/// @brief Process keep alive packet
void pomelo_protocol_client_recv_keep_alive(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_keep_alive_t * packet
);


/* -------------------------------------------------------------------------- */
/*                             Outgoing packets                               */
/* -------------------------------------------------------------------------- */

/// @brief Process sent packet
void pomelo_protocol_client_sent_packet(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer,
    pomelo_protocol_packet_t * packet
);


/* -------------------------------------------------------------------------- */
/*                         Client specific functions                          */
/* -------------------------------------------------------------------------- */

/// @brief Connect to specific address
int pomelo_protocol_client_connect_address(
    pomelo_protocol_client_t * client,
    pomelo_address_t * address
);


/// @brief Send request packet
void pomelo_protocol_client_send_request(pomelo_protocol_client_t * client);


/// @brief Send response packet
void pomelo_protocol_client_send_response(pomelo_protocol_client_t * client);


/// @brief Send keep alive packet
void pomelo_protocol_client_send_keep_alive(pomelo_protocol_client_t * client);


/// @brief Send disconnect packet
void pomelo_protocol_client_send_disconnect(pomelo_protocol_client_t * client);


/// @brief Get the next connecting address of client
/// @return Next address or NULL if there is no more
pomelo_address_t * pomelo_protocol_client_next_address(
    pomelo_protocol_client_t * client
);


/// @brief Disconnect a connected peer
int pomelo_protocol_client_disconnect_peer(
    pomelo_protocol_client_t * client,
    pomelo_protocol_peer_t * peer
);


/// @brief Handle request timeout
void pomelo_protocol_client_handle_request_timeout(
    pomelo_protocol_client_t * client
);


/// @brief Handle disconnect limit
void pomelo_protocol_client_handle_disconnect_limit(
    pomelo_protocol_client_t * client
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_CLIENT_SRC_H
