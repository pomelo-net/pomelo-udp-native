#ifndef POMELO_TOKEN_H
#define POMELO_TOKEN_H
#include <stdint.h>
#include "pomelo/address.h"
#include "pomelo/constants.h"
#ifdef __cplusplus
extern "C" {
#endif


/// The connect token structure
typedef struct pomelo_connect_token_s pomelo_connect_token_t;


struct pomelo_connect_token_s {
    /// @brief 64 bit value unique to this particular game/application
    uint64_t protocol_id;

    /// @brief 64 bit unix timestamp when this connect token was created (ms)
    uint64_t create_timestamp;

    /// @brief 64 bit unix timestamp when this connect token expires (ms)
    uint64_t expire_timestamp;

    /// @brief (24 bytes) The nonce of private connect token
    uint8_t connect_token_nonce[POMELO_CONNECT_TOKEN_NONCE_BYTES];

    /// @brief timeout in seconds. negative values disable timeout (dev only)
    int32_t timeout;
    
    /// @brief Number of server addresses in [1, 32]
    int naddresses;

    /// @brief Array of server addresses
    pomelo_address_t addresses[POMELO_CONNECT_TOKEN_MAX_ADDRESSES];

    /// @brief The key for sending data from client to server
    uint8_t client_to_server_key[POMELO_KEY_BYTES];

    /// @brief The key for sending data from server to client
    uint8_t server_to_client_key[POMELO_KEY_BYTES];

    /* ------------------- Private data of connect token -------------------- */

    /// @brief globally unique identifier for an authenticated client
    int64_t client_id;

    /// @brief The custom user data
    uint8_t user_data[POMELO_USER_DATA_BYTES];
};


/// @brief Encode the connect token information
/// @param buffer The connect token output encrypted buffer
/// @param token The connect token information
/// @param key The key used in encryption
int pomelo_connect_token_encode(
    uint8_t * buffer,
    pomelo_connect_token_t * token,
    uint8_t * key
);


/// @brief Decode the connect token information from connect token
int pomelo_connect_token_decode_public(
    uint8_t * buffer,
    pomelo_connect_token_t * token
);

/// @brief Decode the private part of connect token.
/// The nonce, expire timestamp and protocol id have to be set before involking
/// this function.
/// @param buffer The input buffer (Private part)
/// @param token The output token information
/// @param key The key for decrypting
/// @return 0 on success or an error code < 0 on failure
int pomelo_connect_token_decode_private(
    uint8_t * buffer,
    pomelo_connect_token_t * token,
    uint8_t * key
);


/* -------------------------------------------------------------------------- */
/*                              Codec Utiltities                              */
/* -------------------------------------------------------------------------- */

/// @brief Random value for a buffer
/// @param buffer Output buffer
/// @param length Length of buffer
/// @return 0 on success or an error code < 0 on failure
int pomelo_codec_buffer_random(uint8_t * buffer, size_t length);


/// @brief Encoding variant of base64
/// Source: sodium_base64_VARIANT_URLSAFE
#define pomelo_codec_base64_encoded_variant 5


/// @brief Calculate base64 encoded buffer length
/// Source: sodium_base64_ENCODED_LEN
#define pomelo_codec_base64_calc_encoded_length(bin_len)                       \
    (((bin_len) / 3U) * 4U +                                                   \
    ((((bin_len) - ((bin_len) / 3U) * 3U) |                                    \
    (((bin_len) - ((bin_len) / 3U) * 3U) >> 1)) & 1U) *                        \
    (4U - (~((((pomelo_codec_base64_encoded_variant) & 2U) >> 1) - 1U) &       \
    (3U - ((bin_len) - ((bin_len) / 3U) * 3U)))) + 1U)


/// @brief Encode base64
/// @param b64 Base64 output string
/// @param b64_capacity Capacity of base64 string buffer (include NULL trailing)
/// @param bin Input binary buffer
/// @param bin_size Size of binary buffer
/// @return 0 on success or an error code < 0 on failure
int pomelo_codec_base64_encode(
    char * b64,
    size_t b64_capacity,
    const uint8_t * bin,
    size_t bin_size
);


/// @brief Decode base64
/// @param bin Output binary buffer
/// @param bin_capacity Capacity of binary buffer
/// @param b64 Base64 string
/// @param b64_len Length of base64 string (Not include NULL trail)
/// @return 0 on success or an error code < 0 on failure
int pomelo_codec_base64_decode(
    uint8_t * bin,
    size_t bin_capacity,
    const char * b64,
    size_t b64_len
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_TOKEN_H
