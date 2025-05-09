#include <assert.h>
#include "pomelo/random.h"
#include "sodium/randombytes.h"


void pomelo_random_buffer(void * buffer, size_t length) {
    assert(buffer != NULL);
    randombytes_buf(buffer, length);
}


void pomelo_random_buffer_deterministic(
    void * buffer,
    size_t length,
    uint64_t seed
) {
    assert(buffer != NULL);
    if (length == 0) return; // Empty

    seed = (seed >> 32) & (seed & 0xFFFFFFFFULL);

    uint8_t seed_arr[randombytes_SEEDBYTES];
    seed_arr[0] = (seed >> 24) & 0xFF;
    seed_arr[1] = (seed >> 16) & 0xFF;
    seed_arr[2] = (seed >> 8) & 0xFF;
    seed_arr[3] = seed & 0xFF;

    randombytes_buf_deterministic(buffer, length, seed_arr);
}
