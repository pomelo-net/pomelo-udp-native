#ifndef POMELO_CONSTANTS_SRC_H
#define POMELO_CONSTANTS_SRC_H
#include "pomelo/common.h"


/// The maximum bytes of header
/// + 1 byte for prefix
/// + 1 - 8 bytes for sequence number
#define POMELO_PACKET_HEADER_CAPACITY 9

/// The maximum body size of packet payload. This is set by specification.
#define POMELO_PACKET_PAYLOAD_BODY_CAPACITY_DEFAULT 1200


/// The packet protocol buffer size for packets
/// Warn: Make sure that the capacity of payload packet is the highest
#define POMELO_PACKET_BUFFER_CAPACITY_DEFAULT                                  \
(POMELO_PACKET_HEADER_CAPACITY + POMELO_PACKET_PAYLOAD_BODY_CAPACITY_DEFAULT)


// Version info
#define POMELO_VERSION_INFO "POMELO 1.03"
#define POMELO_VERSION_INFO_BYTES sizeof(POMELO_VERSION_INFO)


// Connect & challenge token
#define POMELO_CONNECT_TOKEN_PRIVATE_BYTES 1024

#define POMELO_CHALLENGE_TOKEN_BYTES 300


// HMAC bytes for packets
#define POMELO_HMAC_BYTES 16



#endif // POMELO_CONSTANTS_SRC_H
