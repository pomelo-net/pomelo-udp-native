#ifndef POMELO_CODEC_PACKET_SRC_H
#define POMELO_CODEC_PACKET_SRC_H
#include "base/packet.h"

#ifdef __cplusplus
extern "C" {
#endif


// Maximum & Minimum number of bytes of sequence numbers
#define POMELO_SEQUENCE_BYTES_MIN 1
#define POMELO_SEQUENCE_BYTES_MAX 8



/// Offset of private part in the connect token.
#define POMELO_CONNECT_TOKEN_PRIVATE_OFFSET ( \
    POMELO_VERSION_INFO_BYTES +               \
    /* Protocol ID */       8 +               \
    /* Create timestamp */  8 +               \
    /* Expire timestamp */  8 +               \
    POMELO_CONNECT_TOKEN_NONCE_BYTES          \
)


/// The codec context
typedef struct pomelo_codec_packet_context_s pomelo_codec_packet_context_t;

/// The packet header information
typedef struct pomelo_codec_packet_header_s pomelo_codec_packet_header_t;


struct pomelo_codec_packet_context_s {
    /// @brief The key for encoding
    uint8_t * packet_encrypt_key;

    /// @brief The key for decoding
    uint8_t * packet_decrypt_key;

    /// @brief The protocol ID for particular game (authenticated peers only)
    uint64_t protocol_id;
};


struct pomelo_codec_packet_header_s {
    /// @brief The packet type
    pomelo_packet_type type;

    /// @brief The sequence number of packet
    uint64_t sequence;

    /// @brief The number of bytes of sequence number
    uint8_t sequence_bytes;
};


/// @brief The codec function type.
/// This function return 0 on success or an error code < 0 on failure
typedef int (*pomelo_codec_packet_fn)(
    pomelo_codec_packet_context_t * context,
    pomelo_packet_t * packet
);


/* -------------------------------------------------------------------------- */
/*                                 Public APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Encode the packet header for packet
int pomelo_codec_encode_packet_header(pomelo_packet_t * packet);

/// @brief Decode the packet header
/// @param header The output header
/// @param payload The input payload
/// @return 0 on success or -1 on failure
int pomelo_codec_decode_packet_header(
    pomelo_codec_packet_header_t * header,
    pomelo_payload_t * payload
);

/// @brief Encrypt the packet body
int pomelo_codec_encrypt_packet(
    pomelo_codec_packet_context_t * context,
    pomelo_packet_t * packet
);

/// @brief Decrypt the packet body
int pomelo_codec_decrypt_packet(
    pomelo_codec_packet_context_t * context,
    pomelo_packet_t * packet
);

/// @brief Common API for decode packet
int pomelo_codec_decode_packet_body(pomelo_packet_t * packet);

/// @brief Common API for encode packet
int pomelo_codec_encode_packet_body(pomelo_packet_t * packet);

/* -------------------------------------------------------------------------- */
/*                                Private APIs                                */
/* -------------------------------------------------------------------------- */

/// @brief Encode the associated data for packets
int pomelo_codec_make_associated_data(
    pomelo_codec_packet_context_t * context,
    uint8_t * buffer,
    uint8_t prefix
);

/// @brief Encode prefix byte
#define pomelo_codec_encode_prefix(packet_type, sequence_bytes)                \
    (uint8_t) ((((packet_type) & 0x0F) << 4) | ((sequence_bytes) & 0x0F))


/// Decode packet type from prefix byte
#define pomelo_codec_decode_prefix_packet_type(prefix) (((prefix) & 0xFF) >> 4)

/// Decode sequence bytes from prefix byte
#define pomelo_codec_decode_prefix_sequence_bytes(prefix) ((prefix) & 0x0F)

/// @brief Encode the request packet with connect token
int pomelo_codec_encode_packet_request_body(pomelo_packet_request_t * packet);

/// @brief Decode the request packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_request_body(pomelo_packet_request_t * packet);

/// @brief Encode the denied packet
int pomelo_codec_encode_packet_denied_body(pomelo_packet_denied_t * packet);

/// @brief Decode the denied packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_denied_body(pomelo_packet_denied_t * packet);

/// @brief Encode the challenge packet
int pomelo_codec_encode_packet_challenge_body(
    pomelo_packet_challenge_t * packet
);

/// @brief Decode the challenge packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_challenge_body(
    pomelo_packet_challenge_t * packet
);

/// @brief Encode the response packet
int pomelo_codec_encode_packet_response_body(pomelo_packet_response_t * packet);

/// @brief Decode the response packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_response_body(pomelo_packet_response_t * packet);

/// @brief Encode the keep alive packet
int pomelo_codec_encode_packet_ping_body(pomelo_packet_ping_t * packet);

/// @brief Decode the keep alive packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_ping_body(pomelo_packet_ping_t * packet);

/// @brief Encode the payload packet
int pomelo_codec_encode_packet_payload_body(pomelo_packet_payload_t * packet);

/// @brief Decode the denied packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_payload_body(pomelo_packet_payload_t * packet);

/// @brief Encode the disconnect packet
int pomelo_codec_encode_packet_disconnect_body(
    pomelo_packet_disconnect_t * packet
);

/// @brief Decode the denied packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_disconnect_body(
    pomelo_packet_disconnect_t * packet
);

/// @brief Encode the disconnect packet
int pomelo_codec_encode_packet_pong_body(pomelo_packet_pong_t * packet);

/// @brief Decode the denied packet
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_codec_decode_packet_pong(pomelo_packet_pong_t * packet);

/// @brief Get the packet prefix
#define pomelo_packet_prefix(packet) packet->header.data[0]

/// @brief Convert token sequence to nonce
int pomelo_codec_packet_sequence_to_nonce(uint8_t * nonce, uint64_t sequence);

#ifdef __cplusplus
}
#endif
#endif // POMELO_CODEC_PACKET_SRC_H
