#ifndef POMELO_PROTOCOL_CRYPTO_SRC_H
#define POMELO_PROTOCOL_CRYPTO_SRC_H
#include <stdint.h>
#include "pomelo/constants.h"
#include "base/ref.h"
#include "base/buffer.h"
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_protocol_crypto_context_s {
    /// @brief The reference of this context
    pomelo_reference_t ref;

    /// @brief The key for encoding
    uint8_t packet_encrypt_key[POMELO_KEY_BYTES];

    /// @brief The key for decoding
    uint8_t packet_decrypt_key[POMELO_KEY_BYTES];

    /// @brief The protocol ID for particular game (authenticated peers only)
    uint64_t protocol_id;

    /// @brief The private key for server
    uint8_t private_key[POMELO_KEY_BYTES];

    /// @brief The challenge key for server
    uint8_t challenge_key[POMELO_KEY_BYTES];
};


/// @brief The on alloc callback for codec context
int pomelo_protocol_crypto_context_on_alloc(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    pomelo_protocol_context_t * context
);


/// @brief Initialize the codec context
int pomelo_protocol_crypto_context_init(
    pomelo_protocol_crypto_context_t * crypto_ctx
);


/// @brief The on finalize callback for codec context
void pomelo_protocol_crypto_context_on_finalize(
    pomelo_protocol_crypto_context_t * crypto_ctx
);


/// @brief Ref the codec context
int pomelo_protocol_crypto_context_ref(
    pomelo_protocol_crypto_context_t * crypto_ctx
);


/// @brief Unref the codec context
int pomelo_protocol_crypto_context_unref(
    pomelo_protocol_crypto_context_t * crypto_ctx
);


/// @brief Decrypt the buffer view
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_protocol_crypto_context_decrypt_packet(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    pomelo_buffer_view_t * view,
    pomelo_protocol_packet_header_t * header
);


/// @brief Encrypt the buffer view
/// @return Returns 0 on success or an error code < 0 on failure
int pomelo_protocol_crypto_context_encrypt_packet(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    pomelo_buffer_view_t * view,
    pomelo_protocol_packet_header_t * header
);


/// @brief Make associated data for packet
void pomelo_protocol_crypto_context_make_associated_data(
    pomelo_protocol_crypto_context_t * crypto_ctx,
    uint8_t * associated_data,
    uint8_t prefix
);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // POMELO_PROTOCOL_CRYPTO_SRC_H
