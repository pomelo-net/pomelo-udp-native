#ifndef POMELO_PROTOCOL_PASS_SRC_H
#define POMELO_PROTOCOL_PASS_SRC_H
#include "protocol.h"
#include "peer.h"
#include "base/packet.h"
#include "platform/platform.h"
#include "utils/list.h"


#ifdef __cplusplus
extern "C" {
#endif


#define POMELO_PROTOCOL_PASS_FLAG_TERMINATED (1 << 0) // Pass is terminated
#define POMELO_PROTOCOL_PASS_FLAG_NO_ENCRYPT (1 << 1) // No encryption
#define POMELO_PROTOCOL_PASS_FLAG_NO_DECRYPT (1 << 2) // No decryption
#define POMELO_PROTOCOL_PASS_FLAG_NO_PROCESS (1 << 3) // No packet processing


/// @brief The sending pass stores the sending information
typedef struct pomelo_protocol_send_pass_s pomelo_protocol_send_pass_t;

/// @brief The receiving pass stores the receiving information
typedef struct pomelo_protocol_recv_pass_s pomelo_protocol_recv_pass_t;


struct pomelo_protocol_recv_pass_s {
    /* ----- Public ----- */

    /// @brief The decode result. 0 on success or an error code < 0 on failure.
    int result;

    /// @brief Flags of receiving pass
    uint8_t flags;

    /// @brief Node of this pass in peer passes list
    pomelo_list_node_t * node;

    /// @brief The packet
    pomelo_packet_t * packet;

    /// @brief The source of packet
    pomelo_protocol_peer_t * peer;

    /// @brief The socket
    pomelo_protocol_socket_t * socket;

    /// @brief Received time
    uint64_t recv_time;
};


struct pomelo_protocol_send_pass_s {
    /* ---------------- Public -----------------*/

    /// @brief Processing result
    int result;

    /// @brief Flags of sending pass
    uint8_t flags;

    /// @brief Node of this pass in peer passes list
    pomelo_list_node_t * node;

    /// @brief The packet to send
    pomelo_packet_t * packet;

    /// @brief The target peer
    pomelo_protocol_peer_t * peer;

    /// @brief The socket of sending pass
    pomelo_protocol_socket_t * socket;
};


/* -------------------------------------------------------------------------- */
/*                              Receiving pass                                */
/* -------------------------------------------------------------------------- */

/// @brief Submit a receiving pass to process
void pomelo_protocol_recv_pass_submit(pomelo_protocol_recv_pass_t * pass);

/// @brief Process the incoming packet
void pomelo_protocol_recv_pass_process(pomelo_protocol_recv_pass_t * pass);

/// @brief Complete callback
void pomelo_protocol_recv_pass_done(pomelo_protocol_recv_pass_t * pass);


/* -------------------------------------------------------------------------- */
/*                               Sending pass                                 */
/* -------------------------------------------------------------------------- */

/// @brief Submit a sending pass to process
void pomelo_protocol_send_pass_submit(pomelo_protocol_send_pass_t * pass);

/// @brief Process the incoming packet
void pomelo_protocol_send_pass_process(pomelo_protocol_send_pass_t * pass);

/// @brief Callback after processing
void pomelo_protocol_send_pass_process_done(pomelo_protocol_send_pass_t * pass);

/// @brief Complete callback, after sending
void pomelo_protocol_send_pass_done(pomelo_protocol_send_pass_t * pass);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_PASS_SRC_H
