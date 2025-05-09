#ifndef POMELO_PACKET_SRC_H
#define POMELO_PACKET_SRC_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "crypto/token.h"
#include "base/payload.h"
#include "base/constants.h"
#include "base/buffer.h"
#include "crypto.h"
#ifdef __cplusplus
extern "C" {
#endif

/// The header size of request packet
#define POMELO_PROTOCOL_PACKET_REQUEST_HEADER_SIZE 1

/// The body size of request packet
#define POMELO_PROTOCOL_PACKET_REQUEST_BODY_SIZE (                             \
    POMELO_VERSION_INFO_BYTES +                                                \
    8 + /* Protocol ID */                                                      \
    8 + /* Expire timestamp */                                                 \
    POMELO_CONNECT_TOKEN_NONCE_BYTES +                                         \
    POMELO_CONNECT_TOKEN_PRIVATE_BYTES                                         \
)

/// The body size of packet body
#define POMELO_PROTOCOL_PACKET_CHALLENGE_BODY_SIZE 308

/// The body size of packet response
#define POMELO_PROTOCOL_PACKET_RESPONSE_BODY_SIZE 308

/// The body size of packet response
#define POMELO_PROTOCOL_PACKET_DENIED_BODY_SIZE 0

/// The body size of packet keep alive
#define POMELO_PROTOCOL_PACKET_KEEP_ALIVE_BODY_SIZE 8

/// The body size of packet disconnect
#define POMELO_PROTOCOL_PACKET_DISCONNECT_BODY_SIZE 0

/// The minimum capacity of encrypted packet
#define POMELO_PROTOCOL_PACKET_ENCRYPTED_MIN_CAPACITY 18

/// The minimum capacity of unencrypted packet
#define POMELO_PROTOCOL_PACKET_UNENCRYPTED_MIN_CAPACITY 2

/// The maximum number of views in payload packet
#define POMELO_PROTOCOL_PAYLOAD_MAX_VIEWS 16

// Maximum & Minimum number of bytes of sequence numbers
#define POMELO_PROTOCOL_SEQUENCE_BYTES_MIN 1
#define POMELO_PROTOCOL_SEQUENCE_BYTES_MAX 8


/// @brief Packet type
typedef enum pomelo_protocol_packet_type {
    POMELO_PROTOCOL_PACKET_REQUEST,
    POMELO_PROTOCOL_PACKET_DENIED,
    POMELO_PROTOCOL_PACKET_CHALLENGE,
    POMELO_PROTOCOL_PACKET_RESPONSE,
    POMELO_PROTOCOL_PACKET_KEEP_ALIVE,
    POMELO_PROTOCOL_PACKET_PAYLOAD,
    POMELO_PROTOCOL_PACKET_DISCONNECT
} pomelo_protocol_packet_type;

/// @brief The number of packet types
#define POMELO_PROTOCOL_PACKET_TYPE_COUNT 7


/// @brief The protocol packet
typedef struct pomelo_protocol_packet_s pomelo_protocol_packet_t;


/// @brief The request packet
typedef struct pomelo_protocol_packet_request_s
    pomelo_protocol_packet_request_t;

/// @brief The request packet info
typedef struct pomelo_protocol_packet_request_info_s
    pomelo_protocol_packet_request_info_t;


/// @brief The challenge packet. It has the same layout as the response packet.
typedef struct pomelo_protocol_packet_challenge_response_s
    pomelo_protocol_packet_challenge_t;

/// @brief The challenge packet info
typedef struct pomelo_protocol_packet_challenge_info_s 
    pomelo_protocol_packet_challenge_info_t;


/// @brief The response packet.
typedef struct pomelo_protocol_packet_challenge_response_s
    pomelo_protocol_packet_response_t;

/// @brief The response packet info
typedef struct pomelo_protocol_packet_response_info_s
    pomelo_protocol_packet_response_info_t;


/// @brief The keep alive packet
typedef struct pomelo_protocol_packet_keep_alive_s
    pomelo_protocol_packet_keep_alive_t;

/// @brief The keep alive packet info
typedef struct pomelo_protocol_packet_keep_alive_info_s
    pomelo_protocol_packet_keep_alive_info_t;


/// @brief The payload packet
typedef struct pomelo_protocol_packet_payload_s
    pomelo_protocol_packet_payload_t;

/// @brief The payload packet info
typedef struct pomelo_protocol_packet_payload_info_s
    pomelo_protocol_packet_payload_info_t;


/// @brief The denied packet
typedef struct pomelo_protocol_packet_denied_s
    pomelo_protocol_packet_denied_t;

typedef struct pomelo_protocol_packet_denied_info_s
    pomelo_protocol_packet_denied_info_t;


/// @brief The disconnect packet
typedef struct pomelo_protocol_packet_disconnect_s
    pomelo_protocol_packet_disconnect_t;

/// @brief The disconnect packet info
typedef struct pomelo_protocol_packet_disconnect_info_s
    pomelo_protocol_packet_disconnect_info_t;


struct pomelo_protocol_packet_header_s {
    /// @brief The prefix byte of packet
    uint8_t prefix;

    /// @brief The packet type
    pomelo_protocol_packet_type type;

    /// @brief The protocol sequence number for packet.
    uint64_t sequence;

    /// @brief The number of bytes of sequence number
    size_t sequence_bytes;
};


struct pomelo_protocol_packet_s {
    /// @brief The packet type
    pomelo_protocol_packet_type type;

    /// @brief The protocol sequence number for packet.
    uint64_t sequence;
};


struct pomelo_protocol_packet_request_s {
    /// @brief Base packet
    pomelo_protocol_packet_t base;

    /// @brief Protocol ID specified by app
    uint64_t protocol_id;

    /// @brief The expired timestamp
    uint64_t expire_timestamp;

    /// @brief The connect token nonce
    uint8_t connect_token_nonce[POMELO_CONNECT_TOKEN_NONCE_BYTES];

    union {
        /// @brief The decrypted data (for server)
        pomelo_connect_token_t token;

        /// @brief The encrypted portion of connect token (for client)
        uint8_t encrypted[POMELO_CONNECT_TOKEN_PRIVATE_BYTES];
    } token_data;
};


struct pomelo_protocol_packet_request_info_s {
    /// @brief The sequence number. This is not used by request packet, but we
    /// add it here to make all packet info structs have the same layout.
    uint64_t sequence;

    /// @brief The protocol ID
    uint64_t protocol_id;

    /// @brief The expired timestamp
    uint64_t expire_timestamp;

    /// @brief The connect token nonce
    uint8_t * connect_token_nonce;

    /// @brief The encrypted portion of connect token
    uint8_t * encrypted_connect_token;
};


struct pomelo_protocol_packet_challenge_response_s {
    /// @brief Base packet
    pomelo_protocol_packet_t base;

    /// @brief The sequence of token (nonce of challenge_token)
    uint64_t token_sequence;

    union {
        /// @brief The challenge token
        pomelo_challenge_token_t token;

        /// @brief The encrypted data of challenge token
        uint8_t encrypted[POMELO_CHALLENGE_TOKEN_BYTES];
    } challenge_data;
};


struct pomelo_protocol_packet_challenge_info_s {
    /// @brief The sequence of challenge packet
    uint64_t sequence;

    /// @brief The sequence of token (nonce of challenge_token)
    uint64_t token_sequence;

    /// @brief The client ID
    int64_t client_id;

    /// @brief The user data
    uint8_t * user_data;
};


struct pomelo_protocol_packet_response_info_s {
    /// @brief The sequence of challenge packet
    uint64_t sequence;

    /// @brief The sequence of token (nonce of challenge_token)
    uint64_t token_sequence;

    /// @brief The encrypted data of challenge token
    uint8_t * encrypted_challenge_token;
};


struct pomelo_protocol_packet_keep_alive_s {
    /// @brief Base packet
    pomelo_protocol_packet_t base;

    /// @brief The client ID
    int64_t client_id;
};


struct pomelo_protocol_packet_keep_alive_info_s {
    /// @brief The sequence number of keep alive packet
    uint64_t sequence;

    /// @brief The client ID
    int64_t client_id;
};


struct pomelo_protocol_packet_payload_s {
    /// @brief Base packet
    pomelo_protocol_packet_t base;

    /// @brief The views of payload data
    pomelo_buffer_view_t views[POMELO_PROTOCOL_PAYLOAD_MAX_VIEWS];

    /// @brief The number of views
    size_t nviews;
};


struct pomelo_protocol_packet_payload_info_s {
    /// @brief The sequence number of payload packet
    uint64_t sequence;

    /// @brief The number of views
    size_t nviews;

    /// @brief The views of payload data
    pomelo_buffer_view_t * views;
};


struct pomelo_protocol_packet_denied_s {
    /// @brief Base packet
    pomelo_protocol_packet_t base;
};


struct pomelo_protocol_packet_denied_info_s {
    /// @brief The sequence number of denied packet
    uint64_t sequence;
};


struct pomelo_protocol_packet_disconnect_s {
    /// @brief Base packet
    pomelo_protocol_packet_t base;
};


struct pomelo_protocol_packet_disconnect_info_s {
    /// @brief The sequence number of disconnect packet
    uint64_t sequence;
};


/// @brief Initialize packet request
int pomelo_protocol_packet_request_init(
    pomelo_protocol_packet_request_t * packet,
    pomelo_protocol_packet_request_info_t * info
);


/// @brief Cleanup packet request
void pomelo_protocol_packet_request_cleanup(
    pomelo_protocol_packet_request_t * packet
);


/// @brief Initialize packet denied
int pomelo_protocol_packet_denied_init(
    pomelo_protocol_packet_denied_t * packet,
    pomelo_protocol_packet_denied_info_t * info
);


/// @brief Cleanup packet denied
void pomelo_protocol_packet_denied_cleanup(
    pomelo_protocol_packet_denied_t * packet
);


/// @brief Initialize packet challenge
int pomelo_protocol_packet_challenge_init(
    pomelo_protocol_packet_challenge_t * packet,
    pomelo_protocol_packet_challenge_info_t * info
);


/// @brief Cleanup packet challenge
void pomelo_protocol_packet_challenge_cleanup(
    pomelo_protocol_packet_challenge_t * packet
);


/// @brief Initialize packet response
int pomelo_protocol_packet_response_init(
    pomelo_protocol_packet_response_t * packet,
    pomelo_protocol_packet_response_info_t * info
);


/// @brief Cleanup packet response
void pomelo_protocol_packet_response_cleanup(
    pomelo_protocol_packet_response_t * packet
);


/// @brief Initialize packet payload
int pomelo_protocol_packet_payload_init(
    pomelo_protocol_packet_payload_t * packet,
    pomelo_protocol_packet_payload_info_t * info
);


/// @brief Cleanup packet payload
void pomelo_protocol_packet_payload_cleanup(
    pomelo_protocol_packet_payload_t * packet
);


/// @brief Attach views to payload packet
void pomelo_protocol_packet_payload_attach_views(
    pomelo_protocol_packet_payload_t * packet,
    pomelo_buffer_view_t * views,
    size_t nviews
);


/// @brief Initialize packet keep alive
int pomelo_protocol_packet_keep_alive_init(
    pomelo_protocol_packet_keep_alive_t * packet,
    pomelo_protocol_packet_keep_alive_info_t * info
);


/// @brief Cleanup packet keep alive
void pomelo_protocol_packet_keep_alive_cleanup(
    pomelo_protocol_packet_keep_alive_t * packet
);


/// @brief Initialize packet disconnect
int pomelo_protocol_packet_disconnect_init(
    pomelo_protocol_packet_disconnect_t * packet,
    pomelo_protocol_packet_disconnect_info_t * info
);


/// @brief Cleanup packet disconnect
void pomelo_protocol_packet_disconnect_cleanup(
    pomelo_protocol_packet_disconnect_t * packet
);


/// @brief Validate the packet body size
bool pomelo_protocol_packet_validate_body_length(
    pomelo_protocol_packet_type type,
    size_t body_length,
    bool encrypted
);


/// @brief Encode prefix byte
#define pomelo_protocol_prefix_encode(packet_type, sequence_bytes)             \
    (uint8_t) ((((packet_type) & 0x0F) << 4) | ((sequence_bytes) & 0x0F))


/// Decode packet type from prefix byte
#define pomelo_protocol_prefix_decode_type(prefix) (((prefix) & 0xFF) >> 4)


/// Decode sequence bytes from prefix byte
#define pomelo_protocol_prefix_decode_sequence_bytes(prefix) ((prefix) & 0x0F)


/// @brief Encode the request packet with connect token
int pomelo_protocol_packet_request_encode(
    pomelo_protocol_packet_request_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Decode the request packet
int pomelo_protocol_packet_request_decode(
    pomelo_protocol_packet_request_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Encode the challenge packet
int pomelo_protocol_packet_challenge_encode(
    pomelo_protocol_packet_challenge_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Decode the challenge packet
int pomelo_protocol_packet_challenge_decode(
    pomelo_protocol_packet_challenge_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Encode the response packet
int pomelo_protocol_packet_response_encode(
    pomelo_protocol_packet_response_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Decode the response packet
int pomelo_protocol_packet_response_decode(
    pomelo_protocol_packet_response_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Encode the keep alive packet
int pomelo_protocol_packet_keep_alive_encode(
    pomelo_protocol_packet_keep_alive_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Decode the keep alive packet
int pomelo_protocol_packet_keep_alive_decode(
    pomelo_protocol_packet_keep_alive_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Encode the payload packet
int pomelo_protocol_packet_payload_encode(
    pomelo_protocol_packet_payload_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Decode the payload packet
int pomelo_protocol_packet_payload_decode(
    pomelo_protocol_packet_payload_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Make header for packet
void pomelo_protocol_packet_header_init(
    pomelo_protocol_packet_header_t * header,
    pomelo_protocol_packet_t * packet
);


/// @brief Encode the packet header for packet
int pomelo_protocol_packet_header_encode(
    pomelo_protocol_packet_header_t * header,
    pomelo_buffer_view_t * view
);


/// @brief Decode the packet header
int pomelo_protocol_packet_header_decode(
    pomelo_protocol_packet_header_t * header,
    pomelo_buffer_view_t * view
);


/// @brief Common API for decode packet
int pomelo_protocol_packet_decode(
    pomelo_protocol_packet_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


/// @brief Common API for encode packet
int pomelo_protocol_packet_encode(
    pomelo_protocol_packet_t * packet,
    pomelo_protocol_crypto_context_t * context,
    pomelo_buffer_view_t * view
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PACKET_SRC_H
