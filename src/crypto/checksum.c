#include <assert.h>
#include "checksum.h"
#include "sodium/crypto_generichash_blake2b.h"


int pomelo_crypto_checksum_init(pomelo_crypto_checksum_state_t * state) {
    assert(state != NULL);

    // Check opaque size
    assert(
        sizeof(pomelo_crypto_checksum_state_t) >=
        sizeof(crypto_generichash_blake2b_state)
    );

    return crypto_generichash_blake2b_init(
        (crypto_generichash_blake2b_state *) state,
        /* key = */ NULL,
        /* key_length = */ 0,
        POMELO_CRYPTO_CHECKSUM_BYTES
    );
}


int pomelo_crypto_checksum_update(
    pomelo_crypto_checksum_state_t * state,
    const uint8_t * buffer,
    size_t length
) {
    assert(state != NULL);
    assert(buffer != NULL);

    return crypto_generichash_blake2b_update(
        (crypto_generichash_blake2b_state *) state,
        buffer,
        length
    );
}


int pomelo_crypto_checksum_final(
    pomelo_crypto_checksum_state_t * state,
    uint8_t * checksum
) {
    assert(state != NULL);
    assert(checksum != NULL);

    return crypto_generichash_blake2b_final(
        (crypto_generichash_blake2b_state *) state,
        checksum,
        POMELO_CRYPTO_CHECKSUM_BYTES
    );
}
