#ifndef POMELO_CRYPTO_CHECKSUM_SRC_H
#define POMELO_CRYPTO_CHECKSUM_SRC_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define POMELO_CRYPTO_CHECKSUM_BYTES 32
#define POMELO_CRYPTO_CHECKSUM_STATE_OPAQUE 384


/// The codec checksum state
typedef struct pomelo_crypto_checksum_state_s {
    uint8_t opaque[POMELO_CRYPTO_CHECKSUM_STATE_OPAQUE];
} pomelo_crypto_checksum_state_t;


/// @brief Initailize the checksum
int pomelo_crypto_checksum_init(pomelo_crypto_checksum_state_t * state);


/// @brief Update the checksum.
/// This will use the capacity of payload as the length of buffer
int pomelo_crypto_checksum_update(
    pomelo_crypto_checksum_state_t * state,
    const uint8_t * buffer,
    size_t length
);


/// @brief Finalize the checksum
int pomelo_crypto_checksum_final(
    pomelo_crypto_checksum_state_t * state,
    uint8_t * output
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_CRYPTO_CHECKSUM_SRC_H
