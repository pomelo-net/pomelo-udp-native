#include <assert.h>
#include "sodium/utils.h"
#include "pomelo/base64.h"


int pomelo_base64_encode(
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


int pomelo_base64_decode(
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
