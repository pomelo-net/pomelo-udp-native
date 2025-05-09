#ifndef POMELO_CONSTANTS_SRC_H
#define POMELO_CONSTANTS_SRC_H
#include "pomelo/common.h"


/// @brief The maximum bytes of header: 1 byte for prefix and [1, 8] bytes for
/// sequence number
#define POMELO_PACKET_HEADER_CAPACITY 9

/// @brief The capacity of a payload. This is set by specification.
#define POMELO_PACKET_BODY_CAPACITY 1200

/// @brief Version info
#define POMELO_VERSION_INFO "POMELO 1.00"

/// @brief Version info size
#define POMELO_VERSION_INFO_BYTES sizeof(POMELO_VERSION_INFO)

// Connect & challenge token
#define POMELO_CONNECT_TOKEN_PRIVATE_BYTES 1024

/// @brief Size of challenge token data
#define POMELO_CHALLENGE_TOKEN_BYTES 300

/// @brief HMAC bytes for packets
#define POMELO_HMAC_BYTES 16

/// @brief Buffer capacity for packets
#define POMELO_BUFFER_CAPACITY (    \
    POMELO_PACKET_HEADER_CAPACITY + \
    POMELO_PACKET_BODY_CAPACITY +   \
    POMELO_HMAC_BYTES               \
)

/// @brief Offset of private part in the connect token.
#define POMELO_CONNECT_TOKEN_PRIVATE_OFFSET ( \
    POMELO_VERSION_INFO_BYTES +               \
    /* Protocol ID */       8 +               \
    /* Create timestamp */  8 +               \
    /* Expire timestamp */  8 +               \
    POMELO_CONNECT_TOKEN_NONCE_BYTES          \
)

#endif // POMELO_CONSTANTS_SRC_H
