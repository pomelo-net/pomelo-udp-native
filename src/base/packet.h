#ifndef POMELO_PACKET_SRC_H
#define POMELO_PACKET_SRC_H
#include <stdint.h>
#include <stdbool.h>
#include "pomelo/token.h"
#include "payload.h"
#include "constants.h"
#include "buffer.h"


#ifdef __cplusplus
extern "C" {
#endif


/// The fixed size of request packet
#define POMELO_PACKET_REQUEST_SIZE (1065 + POMELO_VERSION_INFO_BYTES)

/// The header size of request packet
#define POMELO_PACKET_REQUEST_HEADER_SIZE 1

/// The body size of request packet
#define POMELO_PACKET_REQUEST_BODY_SIZE                                        \
    (POMELO_PACKET_REQUEST_SIZE - POMELO_PACKET_REQUEST_HEADER_SIZE)

/// The body size of packet body
#define POMELO_PACKET_CHALLENGE_BODY_SIZE 308

/// The body size of packet response
#define POMELO_PACKET_RESPONSE_BODY_SIZE 308

/// The body size of packet response
#define POMELO_PACKET_DENIED_BODY_SIZE 0

/// The body size of packet ping
#define POMELO_PACKET_PING_BODY_MIN_SIZE 3
#define POMELO_PACKET_PING_BODY_MAX_SIZE 19

// The body size of packet pong
#define POMELO_PACKET_PONG_BODY_MIN_SIZE 4
#define POMELO_PACKET_PONG_BODY_MAX_SIZE 25

/// The body size of packet disconnect
#define POMELO_PACKET_DISCONNECT_BODY_SIZE 0

/// The minimum capacity of encrypted packet
#define POMELO_PACKET_ENCRYPTED_MIN_CAPACITY 18

/// The minimum capacity of unencrypted packet
#define POMELO_PACKET_UNENCRYPTED_MIN_CAPACITY 2

/// The offset of protocol ID in request packet body
#define POMELO_PACKET_REQUEST_PROTOCOL_ID_OFFSET ( \
    /* prefix */ 1 +                               \
    POMELO_VERSION_INFO_BYTES                      \
)

/// @brief Packet type
typedef enum pomelo_packet_type {
    POMELO_PACKET_REQUEST,
    POMELO_PACKET_DENIED,
    POMELO_PACKET_CHALLENGE,
    POMELO_PACKET_RESPONSE,
    POMELO_PACKET_PING,
    POMELO_PACKET_PAYLOAD,
    POMELO_PACKET_DISCONNECT,
    POMELO_PACKET_PONG,
    POMELO_PACKET_TYPE_COUNT
} pomelo_packet_type;


/// @brief The challenge token
typedef struct pomelo_challenge_token_s pomelo_challenge_token_t;

/// @brief The protocol packet
typedef struct pomelo_packet_s pomelo_packet_t;

/// @brief The request packet
typedef struct pomelo_packet_request_s pomelo_packet_request_t;

/// @brief The challenge packet. It has the same layout as the response packet.
typedef struct pomelo_packet_challenge_s pomelo_packet_challenge_t;

/// @brief The response packet.
typedef struct pomelo_packet_challenge_s pomelo_packet_response_t;

/// @brief The ping packet
typedef struct pomelo_packet_ping_s pomelo_packet_ping_t;

/// @brief The payload packet
typedef struct pomelo_packet_payload_s pomelo_packet_payload_t;

/// @brief The denied packet
typedef struct pomelo_packet_s pomelo_packet_denied_t;

/// @brief The disconnect packet
typedef struct pomelo_packet_s pomelo_packet_disconnect_t;

/// @brief The pong packet
typedef struct pomelo_packet_pong_s pomelo_packet_pong_t;


struct pomelo_packet_s {
    /// @brief The packet type
    pomelo_packet_type type;

    /// @brief The protocol sequence number for packet.
    uint64_t sequence;

    /// @brief The header payload
    pomelo_payload_t header;

    /// @brief The body payload
    pomelo_payload_t body;

    /// @brief Buffer which is used by this packet
    pomelo_buffer_t * buffer;
};


struct pomelo_packet_request_s {
    /// @brief Base packet
    pomelo_packet_t base;

    /// @brief Protocol ID specified by app
    uint64_t protocol_id;

    /// @brief The expired timestamp
    uint64_t expire_timestamp;

    /// @brief The connect token nonce
    uint8_t connect_token_nonce[POMELO_CONNECT_TOKEN_NONCE_BYTES];

    /// @brief Private key for decoding token
    uint8_t * private_key;

    union {
        /// @brief The decrypted data (for server)
        pomelo_connect_token_t token;

        /// @brief The encrypted data (for client)
        uint8_t encrypted_token[POMELO_CONNECT_TOKEN_PRIVATE_BYTES];
    };
};


struct pomelo_challenge_token_s {
    /// @brief The client ID
    int64_t client_id;

    /// @brief The user data
    uint8_t user_data[POMELO_USER_DATA_BYTES];
};


struct pomelo_packet_challenge_s {
    /// @brief Base packet
    pomelo_packet_t base;

    /// @brief The sequence of token (nonce of challenge_token)
    uint64_t token_sequence;

    /// @brief Challenge key for encrypting
    uint8_t * challenge_key;

    union {
        /// @brief The challenge token
        pomelo_challenge_token_t challenge_token;

        /// @brief The encrypted data of challenge token
        uint8_t encrypted_challenge_token[POMELO_CHALLENGE_TOKEN_BYTES];
    };
};


struct pomelo_packet_ping_s {
    /// @brief Base packet
    pomelo_packet_t base;

    /// @brief The client ID
    int64_t client_id;

    /// @brief The sequence number of ping response
    uint64_t ping_sequence;

    /// @brief Attach time flag
    bool attach_time;

    /// @brief Time of server
    uint64_t time;
};


struct pomelo_packet_pong_s {
    /// @brief Base packet
    pomelo_packet_t base;

    /// @brief The sequence number of ping response
    uint64_t ping_sequence;

    /// @brief Time when received ping
    uint64_t ping_recv_time;

    /// @brief Delta time from ping received time to sending time of pong packet
    uint64_t pong_delta_time;
};


struct pomelo_packet_payload_s {
    /// @brief Base packet
    pomelo_packet_t base;

    /// @brief The source of payload
    pomelo_buffer_t * source;
};


/// @brief Init packet request
void pomelo_packet_request_init(pomelo_packet_request_t * packet);

/// @brief Init packet response
void pomelo_packet_response_init(pomelo_packet_response_t * packet);

/// @brief Init packet denied
void pomelo_packet_denied_init(pomelo_packet_denied_t * packet);

/// @brief Init packet challenge
void pomelo_packet_challenge_init(pomelo_packet_challenge_t * packet);

/// @brief Init packet payload
void pomelo_packet_payload_init(pomelo_packet_payload_t * packet);

/// @brief Init packet keep alive
void pomelo_packet_ping_init(pomelo_packet_ping_t * packet);

/// @brief Init packet disconnect
void pomelo_packet_disconnect_init(pomelo_packet_disconnect_t * packet);

/// @brief Init packet ping
void pomelo_packet_pong_init(pomelo_packet_pong_t * packet);

/// @brief Reset the packet
void pomelo_packet_reset(pomelo_packet_t * packet);

/// @brief Attach buffer to packet and locate the header & body of buffer
void pomelo_packet_attach_buffer(
    pomelo_packet_t * packet,
    pomelo_buffer_t * buffer
);

/// @brief Validate the packet body size
bool pomelo_packet_validate_packet_body_size(
    pomelo_packet_type type,
    size_t body_size
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_PACKET_SRC_H
