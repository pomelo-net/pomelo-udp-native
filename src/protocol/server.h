#ifndef POMELO_PROTOCOL_SERVER_SRC_H
#define POMELO_PROTOCOL_SERVER_SRC_H
#include <stdbool.h>
#include "protocol.h"
#include "platform/platform.h"
#include "base/packet.h"
#include "utils/pool.h"
#include "utils/map.h"
#include "socket.h"


/// The expire time of anonymous peers (60 seconds)
#define POMELO_ANONYMOUS_PEER_EXPIRE_TIME_NS 60000000000ULL

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_protocol_server_s {
    /// @brief The base socket
    pomelo_protocol_socket_t socket;

    /// @brief The peers pool
    pomelo_pool_t * peer_pool;

    /// @brief The address map for connected peers.
    /// Map from address to peer.
    pomelo_map_t * address_connected_map;

    /// @brief The address map for anonymous peers.
    /// Map from address to peer
    pomelo_map_t * address_anonymous_map;

    /// @brief The client id map. Map from client ID to peer
    pomelo_map_t * client_id_map;

    /// @brief The connected peers
    pomelo_list_t * connected_peers;

    /// @brief The disconnecting peers
    pomelo_list_t * disconnecting_peers;

    /// @brief The anonymous peers waiting to be authenticated
    pomelo_list_t * anonymous_peers;

    /// @brief The private key for decoding request packets
    uint8_t private_key[POMELO_KEY_BYTES];
    
    /// @brief The challenge key
    uint8_t challenge_key[POMELO_KEY_BYTES];

    /// @brief The maximum number of clients
    size_t max_clients;

    /// @brief The protocol ID of server
    uint64_t protocol_id;

    /// @brief The bind address of server
    pomelo_address_t address;

    /// @brief The ping timer
    pomelo_platform_timer_t * ping_timer;

    /// @brief The sequence number for anonymous peer
    uint64_t anonymous_sequence_number;

    /// @brief The challenge packet sequence number
    uint64_t challenge_sequence_number;

    /// @brief The timer for sending redundants disconnect packets
    pomelo_platform_timer_t * disconnecting_timer;
};



/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */


/// Destroy all pools & maps of server
void pomelo_protocol_server_destroy(pomelo_protocol_server_t * server);

/// @brief The socket server starts listening
/// @return 0 on success, or an error code < 0 on failure
int pomelo_protocol_server_start(pomelo_protocol_server_t * server);

/// @brief Stop the server
void pomelo_protocol_server_stop(pomelo_protocol_server_t * server);

/// @brief Deferred stopping server
void pomelo_protocol_server_stop_deferred(pomelo_protocol_server_t * server);

/* -------------------------------------------------------------------------- */
/*                            Receiving callbacks                             */
/* -------------------------------------------------------------------------- */

/// @brief Receiving callback for socket server
int pomelo_protocol_server_on_recv(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address,
    pomelo_buffer_t * buffer,
    size_t length,
    uint64_t recv_time,
    bool encrypted
);


/* -------------------------------------------------------------------------- */
/*                          Server incoming packets                           */
/* -------------------------------------------------------------------------- */

/// @brief Process request packet
void pomelo_protocol_server_recv_request_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * packet,
    int result
);

/// @brief Process the response packet
void pomelo_protocol_server_recv_response_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_response_t * packet,
    int result
);

/// @brief Process disconnect packet
void pomelo_protocol_server_recv_disconnect_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
);

/// @brief Process ping packet
void pomelo_protocol_server_recv_ping_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    uint64_t recv_time,
    int result
);


/* -------------------------------------------------------------------------- */
/*                          Server outgoing packets                           */
/* -------------------------------------------------------------------------- */


/// @brief Denied packet sending callback. This will cleanup the
/// anonymous peer.
void pomelo_protocol_server_sent_denied_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_denied_t * packet,
    int result
);


/// @brief Challenge packet sending callback
void pomelo_protocol_server_sent_challenge_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_challenge_t * packet,
    int result
);

/// @brief ping packet sending callback
void pomelo_protocol_server_sent_ping_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_ping_t * packet,
    int result
);

/// @brief Disconnect packet sending callback
void pomelo_protocol_server_sent_disconnect_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_disconnect_t * packet,
    int result
);


/* -------------------------------------------------------------------------- */
/*                        Server specific functions                           */
/* -------------------------------------------------------------------------- */


/// @brief The ping callback for socket server
void pomelo_protocol_server_ping_callback(
    pomelo_protocol_server_t * server
);

/// @brief Process disconnected peer for server socket
void pomelo_protocol_server_process_disconnected_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
);

/// @brief Send the connection challenge packet
void pomelo_protocol_server_send_challenge_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer,
    pomelo_packet_request_t * request_packet
);


/// @brief Send the connection denied packet.
/// The anonymous peer will automatically removed after sending is done.
void pomelo_protocol_server_send_denied_packet(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
);

/// @brief Send ping packet to target peer
void pomelo_protocol_server_ping(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
);

/// @brief Get the connected peer by address
pomelo_protocol_peer_t * pomelo_protocol_server_find_connected_peer(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address
);

/// @brief Get the anonymous peer by address
pomelo_protocol_peer_t * pomelo_protocol_server_find_anonymous_peer(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address
);

/// @brief Find or create new anonymous peer
pomelo_protocol_peer_t * pomelo_protocol_server_find_or_create_anonymous_peer(
    pomelo_protocol_server_t * server,
    pomelo_address_t * address
);

/// @brief Remove a connected peer
void pomelo_protocol_server_remove_connected_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
);

/// @brief Remove an anonymous peer
void pomelo_protocol_server_remove_anonymous_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
);


/// @brief Disconnect a connected peer
int pomelo_protocol_server_disconnect_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
);

/// @brief Send redundant disconnect packets to all disconnecting peers
void pomelo_protocol_server_send_disconnect(pomelo_protocol_server_t * server);

/// @brief Send a redundant disconnect packet to a peer
void pomelo_protocol_server_send_disconnect_peer(
    pomelo_protocol_server_t * server,
    pomelo_protocol_peer_t * peer
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_SERVER_SRC_H
