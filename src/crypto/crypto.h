#ifndef POMELO_CRYPTO_SRC_H
#define POMELO_CRYPTO_SRC_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif


/// @brief The length of nonce
#define POMELO_CRYPTO_AEAD_NONCE_BYTES  12

/// @brief The length of HMAC
#define POMELO_CRYPTO_AEAD_HMAC_BYTES   16


/// @brief Initialize crypto system
int pomelo_crypto_init(void);


/// @brief Make nonce from sequence number
void pomelo_crypto_make_nonce(
    uint8_t * nonce,
    size_t nonce_length,
    uint64_t sequence
);


/// @brief Encrypt the input buffer with AEAD
int pomelo_crypto_encrypt_aead(
    uint8_t * output,
    size_t * output_length,
    const uint8_t * input,
    size_t input_length,
    const uint8_t * key,
    const uint8_t * nonce,
    const uint8_t * ad,
    size_t adlen
);


/// @brief Decrypt the input buffer with AEAD
int pomelo_crypto_decrypt_aead(
    uint8_t * output,
    size_t * output_length,
    const uint8_t * input,
    size_t input_length,
    const uint8_t * key,
    const uint8_t * nonce,
    const uint8_t * ad,
    size_t adlen
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_CRYPTO_SRC_H
