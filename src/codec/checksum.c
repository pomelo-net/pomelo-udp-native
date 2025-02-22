#include <assert.h>
#include "checksum.h"
#include "sodium/crypto_generichash_blake2b.h"


int pomelo_codec_checksum_init(pomelo_codec_checksum_state_t * state) {
    assert(state != NULL);

    // Check opaque size
    assert(
        sizeof(pomelo_codec_checksum_state_t) >=
        sizeof(crypto_generichash_blake2b_state)
    );

    return crypto_generichash_blake2b_init(
        (crypto_generichash_blake2b_state *) state,
        /* key = */ NULL,
        /* key_length = */ 0,
        POMELO_CODEC_CHECKSUM_BYTES
    );
}


int pomelo_codec_checksum_update(
    pomelo_codec_checksum_state_t * state,
    pomelo_payload_t * payload
) {
    assert(state != NULL);
    assert(payload != NULL);

    return crypto_generichash_blake2b_update(
        (crypto_generichash_blake2b_state *) state,
        payload->data + payload->position,
        payload->capacity - payload->position
    );
}


int pomelo_codec_checksum_final(
    pomelo_codec_checksum_state_t * state,
    uint8_t * checksum
) {
    assert(state != NULL);
    assert(checksum != NULL);

    return crypto_generichash_blake2b_final(
        (crypto_generichash_blake2b_state *) state,
        checksum,
        POMELO_CODEC_CHECKSUM_BYTES
    );
}
