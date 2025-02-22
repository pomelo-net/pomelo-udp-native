#include <assert.h>
#include <stdbool.h>
#include "codec.h"
#include "sodium/core.h"
#include "sodium/utils.h"
#include "sodium/randombytes.h"


/// @brief Initialized flag
static bool _initialized = false;


int pomelo_codec_init(void) {
    if (_initialized) {
        return 0;
    }

    int ret = sodium_init();
    if (ret == 0) {
        _initialized = true;
    }
    return ret;
}


/* -------------------------------------------------------------------------- */
/*                              Codec Utiltities                              */
/* -------------------------------------------------------------------------- */

int pomelo_codec_buffer_random(uint8_t * buffer, size_t length) {
    assert(buffer != NULL);
    randombytes_buf(buffer, length);
    return 0;
}


int pomelo_codec_base64_encode(
    char * b64,
    size_t b64_capacity,
    const uint8_t * bin,
    size_t bin_size
) {
    assert(b64 != NULL);
    assert(bin != NULL);

    sodium_bin2base64(
        b64,
        b64_capacity,
        bin,
        bin_size,
        sodium_base64_VARIANT_URLSAFE
    );

    return 0;
}


int pomelo_codec_base64_decode(
    uint8_t * bin,
    size_t bin_capacity,
    const char * b64,
    size_t b64_len
) {
    assert(b64 != NULL);
    assert(bin != NULL);

    return sodium_base642bin(
        bin,
        bin_capacity,
        b64,
        b64_len,
        NULL,
        NULL,
        NULL,
        (b64_len % 4 == 0)
            ? sodium_base64_VARIANT_URLSAFE
            : sodium_base64_VARIANT_URLSAFE_NO_PADDING
    );
}
