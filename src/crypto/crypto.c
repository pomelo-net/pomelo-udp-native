#include <assert.h>
#include <stdbool.h>
#include "crypto.h"
#include "base/payload.h"
#include "sodium/core.h"
#include "sodium/utils.h"
#include "sodium/randombytes.h"
#include "sodium/crypto_aead_chacha20poly1305.h"


/// @brief Initialized flag
static bool _initialized = false;


int pomelo_crypto_init(void) {
    if (_initialized) {
        return 0;
    }

    int ret = sodium_init();
    if (ret == 0) {
        _initialized = true;
    }
    return ret;
}


void pomelo_crypto_make_nonce(
    uint8_t * nonce,
    size_t nonce_length,
    uint64_t sequence
) {
    assert(nonce != NULL);

    pomelo_payload_t payload;
    payload.data = nonce;
    payload.position = 0;
    payload.capacity = nonce_length;

    // padding high bits with zero
    pomelo_payload_zero_pad_unsafe(&payload, nonce_length - sizeof(uint64_t));

    // write the token sequence
    pomelo_payload_write_uint64_unsafe(&payload, sequence);
}


int pomelo_crypto_encrypt_aead(
    uint8_t * output,
    size_t * output_length,
    const uint8_t * input,
    size_t input_length,
    const uint8_t * key,
    const uint8_t * nonce,
    const uint8_t * ad,
    size_t adlen
) {
    assert(output != NULL);
    assert(input != NULL);
    assert(key != NULL);
    assert(nonce != NULL);
    assert(ad != NULL);

    unsigned long long tmp_output_length = 0;
    int ret = crypto_aead_chacha20poly1305_ietf_encrypt(
        output,
        &tmp_output_length,
        input,
        input_length,
        ad,
        adlen,
        NULL,
        nonce,
        key
    );
    if (ret < 0) return ret;

    if (output_length != NULL) {
        *output_length = tmp_output_length;
    }

    return 0;
}


int pomelo_crypto_decrypt_aead(
    uint8_t * output,
    size_t * output_length,
    const uint8_t * input,
    size_t input_length,
    const uint8_t * key,
    const uint8_t * nonce,
    const uint8_t * ad,
    size_t adlen
) {
    assert(output != NULL);
    assert(input != NULL);
    assert(key != NULL);
    assert(nonce != NULL);
    assert(ad != NULL);

    unsigned long long tmp_output_length = 0;
    int ret = crypto_aead_chacha20poly1305_ietf_decrypt(
        output,
        &tmp_output_length,
        NULL,
        input,
        input_length,
        ad,
        adlen,
        nonce,
        key
    );
    if (ret < 0) return ret;

    if (output_length != NULL) {
        *output_length = tmp_output_length;
    }

    return 0;
}
