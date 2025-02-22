#include <assert.h>
#include "packed.h"


size_t pomelo_codec_calc_packed_uint64_bytes(uint64_t value) {
    if (value & 0xFFFFFFFF00000000ULL) {
        // Need > 4 bytes
        if (value & 0xFFFF000000000000ULL) {
            // Need > 6 bytes
            if (value & 0xFF000000000000ULL) {
                return 8; // Need 8 bytes
            } else {
                return 7; // Need 7 bytes
            }
        } else {
            // Need <= 6 bytes
            if (value & 0xFFFFFF0000000000ULL) {
                return 6; // Need 6 bytes
            } else {
                return 5; // Need 5 bytes
            }
        }
    } else {
        // Need <= 4 bytes
        if (value & 0xFFFF0000ULL) {
            // Need > 2 bytes
            if (value & 0xFF000000ULL) {
                return 4; // Need 4 bytes
            } else {
                return 3; // Need 3 bytes
            }
        } else {
            // Need <= 2 bytes
            if (value & 0xFF00ULL) {
                return 2; // Need 2 bytes
            } else {
                return 1; // Need 1 byte
            }
        }
    }
}


int pomelo_codec_read_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t * value
) {
    assert(payload != NULL);
    assert(value != NULL);

    *value = 0;
    uint8_t byte = 0;
    for (size_t i = 0; i < bytes; i++) {
        int ret = pomelo_payload_read_uint8(payload, &byte);
        if (ret < 0) {
            return ret;
        }

        *value |= ((uint64_t) byte) << (i * 8);
    }

    return 0;
}


int pomelo_codec_write_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t value
) {
    assert(payload != NULL);

    // sequence bytes
    for (size_t i = 0; i < bytes; ++i) {
        int ret = pomelo_payload_write_uint8(payload, value & 0xFF);
        if (ret < 0) {
            return ret;
        }

        value >>= 8;
    }

    return 0;
}
