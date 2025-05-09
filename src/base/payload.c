#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "payload.h"


/* -------------------------------------------------------------------------- */
/*                               Public API                                   */
/* -------------------------------------------------------------------------- */

int pomelo_payload_write_uint8(pomelo_payload_t * payload, uint8_t value) {
    assert(payload != NULL);

    if (payload->position + 1 > payload->capacity) {
        return -1;
    }

    payload->data[payload->position++] = value;
    return 0;
}


int pomelo_payload_write_uint16(pomelo_payload_t * payload, uint16_t value) {
    assert(payload != NULL);

    size_t position = payload->position;
    if (position + 2 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;

    data[position] = (uint8_t) value;
    data[position + 1] = (uint8_t) (value >> 8);

    payload->position += 2;
    return 0;
}


int pomelo_payload_write_uint32(pomelo_payload_t * payload, uint32_t value) {
    assert(payload != NULL);

    size_t position = payload->position;
    if (position + 4 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;

    data[position] = (uint8_t) value;
    data[position + 1] = (uint8_t) (value >> 8);
    data[position + 2] = (uint8_t) (value >> 16);
    data[position + 3] = (uint8_t) (value >> 24);

    payload->position += 4;
    return 0;
}


int pomelo_payload_write_uint64(pomelo_payload_t * payload, uint64_t value) {
    assert(payload != NULL);

    size_t position = payload->position;
    if (position + 8 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;

    data[position] = (uint8_t) value;
    data[position + 1] = (uint8_t) (value >> 8);
    data[position + 2] = (uint8_t) (value >> 16);
    data[position + 3] = (uint8_t) (value >> 24);
    data[position + 4] = (uint8_t) (value >> 32);
    data[position + 5] = (uint8_t) (value >> 40);
    data[position + 6] = (uint8_t) (value >> 48);
    data[position + 7] = (uint8_t) (value >> 56);

    payload->position += 8;
    return 0;
}


int pomelo_payload_write_int8(pomelo_payload_t * payload, int8_t value) {
    return pomelo_payload_write_uint8(payload, value);
}


int pomelo_payload_write_int16(pomelo_payload_t * payload, int16_t value) {
    return pomelo_payload_write_uint16(payload, value);
}


int pomelo_payload_write_int32(pomelo_payload_t * payload, int32_t value) {
    return pomelo_payload_write_uint32(payload, value);
}


int pomelo_payload_write_int64(pomelo_payload_t * payload, int64_t value) {
    return pomelo_payload_write_uint64(payload, value);
}


int pomelo_payload_write_float32(pomelo_payload_t * payload, float value) {
    uint32_t temp;
    memcpy(&temp, &value, sizeof(float));  // Use memcpy to avoid strict aliasing violations
    return pomelo_payload_write_uint32(payload, temp);
}


int pomelo_payload_write_float64(pomelo_payload_t * payload, double value) {
    uint64_t temp;
    memcpy(&temp, &value, sizeof(double));  // Use memcpy to avoid strict aliasing violations
    return pomelo_payload_write_uint64(payload, temp);
}


int pomelo_payload_write_buffer(
    pomelo_payload_t * payload,
    const uint8_t * buffer,
    size_t size
) {
    assert(payload != NULL);
    assert(buffer != NULL);

    if (size == 0) {
        // Nothing to do, but this is a successful case
        return 0;
    }

    if (payload->position + size > payload->capacity) {
        return -1;
    }
    memcpy(payload->data + payload->position, buffer, size);
    payload->position += size;
    return 0;
}


int pomelo_payload_zero_pad(pomelo_payload_t * payload, size_t pad_size) {
    assert(payload != NULL);

    if (pad_size > payload->capacity) {
        return -1;
    }

    size_t position = payload->position;
    if (pad_size <= position) {
        return 0;
    }

    // Check for potential overflow
    size_t remain = pad_size - position;
    if (remain > payload->capacity - position) {
        return -1;
    }

    memset(payload->data + position, 0, remain);
    payload->position = pad_size;
    return 0;
}


int pomelo_payload_read_uint8(pomelo_payload_t * payload, uint8_t * value) {
    assert(payload != NULL);
    assert(value);

    if (payload->position + 1 > payload->capacity) {
        return -1;
    }

    *value = payload->data[payload->position++];
    return 0;
}


int pomelo_payload_read_uint16(pomelo_payload_t * payload, uint16_t * value) {
    assert(payload != NULL);
    assert(value);

    size_t position = payload->position;
    if (position + 2 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;
    *value = (uint16_t) data[position] |
             ((uint16_t) data[position + 1] << 8);
             
    payload->position += 2;
    return 0;
}


int pomelo_payload_read_uint32(pomelo_payload_t * payload, uint32_t * value) {
    assert(payload != NULL);
    assert(value);

    size_t position = payload->position;
    if (position + 4 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;
    *value = (uint32_t) data[position] |
             ((uint32_t) data[position + 1] << 8) |
             ((uint32_t) data[position + 2] << 16) |
             ((uint32_t) data[position + 3] << 24);

    payload->position += 4;
    return 0;
}


int pomelo_payload_read_uint64(pomelo_payload_t * payload, uint64_t * value) {
    assert(payload != NULL);
    assert(value);

    size_t position = payload->position;
    if (position + 8 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;

    *value = (uint64_t) data[position] |
             ((uint64_t) data[position + 1] << 8) |
             ((uint64_t) data[position + 2] << 16) |
             ((uint64_t) data[position + 3] << 24) |
             ((uint64_t) data[position + 4] << 32) |
             ((uint64_t) data[position + 5] << 40) |
             ((uint64_t) data[position + 6] << 48) |
             ((uint64_t) data[position + 7] << 56);

    payload->position += 8;
    return 0;
}


int pomelo_payload_read_int8(pomelo_payload_t * payload, int8_t * value) {
    return pomelo_payload_read_uint8(payload, (uint8_t *) value);
}


int pomelo_payload_read_int16(pomelo_payload_t * payload, int16_t * value) {
    return pomelo_payload_read_uint16(payload, (uint16_t *) value);
}


int pomelo_payload_read_int32(pomelo_payload_t * payload, int32_t * value) {
    return pomelo_payload_read_uint32(payload, (uint32_t *) value);
}


int pomelo_payload_read_int64(pomelo_payload_t * payload, int64_t * value) {
    return pomelo_payload_read_uint64(payload, (uint64_t *) value);
}


int pomelo_payload_read_float32(pomelo_payload_t * payload, float * value) {
    uint32_t temp;
    int result = pomelo_payload_read_uint32(payload, &temp);
    if (result == 0) {
        memcpy(value, &temp, sizeof(float));  // Use memcpy to avoid strict aliasing violations
    }
    return result;
}


int pomelo_payload_read_float64(pomelo_payload_t * payload, double * value) {
    uint64_t temp;
    int result = pomelo_payload_read_uint64(payload, &temp);
    if (result == 0) {
        memcpy(value, &temp, sizeof(double));  // Use memcpy to avoid strict aliasing violations
    }
    return result;
}


int pomelo_payload_read_buffer(
    pomelo_payload_t * payload,
    uint8_t * buffer,
    size_t size
) {
    assert(payload != NULL);
    assert(buffer != NULL);

    if (payload->position + size > payload->capacity) {
        return -1;
    }
    memcpy(buffer, payload->data + payload->position, size);
    payload->position += size;
    return 0;
}


/* -------------------------------------------------------------------------- */
/*                           Unsafe functions                                 */
/* -------------------------------------------------------------------------- */

void pomelo_payload_write_uint8_unsafe(
    pomelo_payload_t * payload,
    uint8_t value
) {
    assert(payload != NULL);
    payload->data[payload->position++] = value;
}


void pomelo_payload_write_uint16_unsafe(
    pomelo_payload_t * payload,
    uint16_t value
) {
    assert(payload != NULL);
    size_t position = payload->position;
    uint8_t * data = payload->data;

    data[position] = (uint8_t) value;
    data[position + 1] = (uint8_t) (value >> 8);

    payload->position += 2;
}


void pomelo_payload_write_uint32_unsafe(
    pomelo_payload_t * payload,
    uint32_t value
) {
    assert(payload != NULL);
    size_t position = payload->position;
    uint8_t * data = payload->data;

    data[position] = (uint8_t) value;
    data[position + 1] = (uint8_t) (value >> 8);
    data[position + 2] = (uint8_t) (value >> 16);
    data[position + 3] = (uint8_t) (value >> 24);

    payload->position += 4;
}


void pomelo_payload_write_uint64_unsafe(
    pomelo_payload_t * payload,
    uint64_t value
) {
    assert(payload != NULL);
    size_t position = payload->position;
    uint8_t * data = payload->data;

    data[position] = (uint8_t) value;
    data[position + 1] = (uint8_t) (value >> 8);
    data[position + 2] = (uint8_t) (value >> 16);
    data[position + 3] = (uint8_t) (value >> 24);
    data[position + 4] = (uint8_t) (value >> 32);
    data[position + 5] = (uint8_t) (value >> 40);
    data[position + 6] = (uint8_t) (value >> 48);
    data[position + 7] = (uint8_t) (value >> 56);

    payload->position += 8;
}


void pomelo_payload_write_int8_unsafe(
    pomelo_payload_t * payload,
    int8_t value
) {
    pomelo_payload_write_uint8_unsafe(payload, (uint8_t) value);
}


void pomelo_payload_write_int16_unsafe(
    pomelo_payload_t * payload,
    int16_t value
) {
    pomelo_payload_write_uint16_unsafe(payload, (uint16_t) value);
}


void pomelo_payload_write_int32_unsafe(
    pomelo_payload_t * payload,
    int32_t value
) {
    pomelo_payload_write_uint32_unsafe(payload, (uint32_t) value);
}


void pomelo_payload_write_int64_unsafe(
    pomelo_payload_t * payload,
    int64_t value
) {
    pomelo_payload_write_uint64_unsafe(payload, (uint64_t) value);
}


void pomelo_payload_write_float32_unsafe(
    pomelo_payload_t * payload,
    float value
) {
    uint32_t temp;
    memcpy(&temp, &value, sizeof(float));  // Fix the unsafe version as well
    pomelo_payload_write_uint32_unsafe(payload, temp);
}


void pomelo_payload_write_float64_unsafe(
    pomelo_payload_t * payload,
    double value
) {
    uint64_t temp;
    memcpy(&temp, &value, sizeof(double));  // Fix the unsafe version as well
    pomelo_payload_write_uint64_unsafe(payload, temp);
}


void pomelo_payload_write_buffer_unsafe(
    pomelo_payload_t * payload,
    const uint8_t * buffer,
    size_t size
) {
    assert(payload != NULL);
    assert(buffer != NULL);
    memcpy(payload->data + payload->position, buffer, size);
    payload->position += size;
}


void pomelo_payload_zero_pad_unsafe(
    pomelo_payload_t * payload,
    size_t pad_size
) {
    assert(payload != NULL);
    assert(payload->position + pad_size <= payload->capacity);  // Add safety check
    memset(payload->data + payload->position, 0, pad_size);
    payload->position += pad_size;
}


void pomelo_payload_read_uint8_unsafe(
    pomelo_payload_t * payload,
    uint8_t * value
) {
    assert(payload != NULL);
    assert(value);
    *value = payload->data[payload->position++];
}


void pomelo_payload_read_uint16_unsafe(
    pomelo_payload_t * payload,
    uint16_t * value
) {
    assert(payload != NULL);
    assert(value);
    size_t position = payload->position;
    uint8_t * data = payload->data;

    *value = (uint16_t) data[position] |
             ((uint16_t) data[position + 1] << 8);

    payload->position += 2;
}


void pomelo_payload_read_uint32_unsafe(
    pomelo_payload_t * payload,
    uint32_t * value
) {
    assert(payload != NULL);
    assert(value);
    size_t position = payload->position;
    uint8_t * data = payload->data;

    *value = (uint32_t) data[position] |
             ((uint32_t) data[position + 1] << 8) |
             ((uint32_t) data[position + 2] << 16) |
             ((uint32_t) data[position + 3] << 24);

    payload->position += 4;
}


void pomelo_payload_read_uint64_unsafe(
    pomelo_payload_t * payload,
    uint64_t * value
) {
    assert(payload != NULL);
    assert(value);
    size_t position = payload->position;
    uint8_t * data = payload->data;

    *value = (uint64_t) data[position] |
             ((uint64_t) data[position + 1] << 8) |
             ((uint64_t) data[position + 2] << 16) |
             ((uint64_t) data[position + 3] << 24) |
             ((uint64_t) data[position + 4] << 32) |
             ((uint64_t) data[position + 5] << 40) |
             ((uint64_t) data[position + 6] << 48) |
             ((uint64_t) data[position + 7] << 56);

    payload->position += 8;
}


void pomelo_payload_read_int8_unsafe(
    pomelo_payload_t * payload,
    int8_t * value
) {
    pomelo_payload_read_uint8_unsafe(payload, (uint8_t *) value);
}


void pomelo_payload_read_int16_unsafe(
    pomelo_payload_t * payload,
    int16_t * value
) {
    pomelo_payload_read_uint16_unsafe(payload, (uint16_t *) value);
}


void pomelo_payload_read_int32_unsafe(
    pomelo_payload_t * payload,
    int32_t * value
) {
    pomelo_payload_read_uint32_unsafe(payload, (uint32_t *) value);
}


void pomelo_payload_read_int64_unsafe(
    pomelo_payload_t * payload,
    int64_t * value
) {
    pomelo_payload_read_uint64_unsafe(payload, (uint64_t *) value);
}


void pomelo_payload_read_float32_unsafe(
    pomelo_payload_t * payload,
    float * value
) {
    uint32_t temp;
    pomelo_payload_read_uint32_unsafe(payload, &temp);
    memcpy(value, &temp, sizeof(float));  // Fix the unsafe version as well
}


void pomelo_payload_read_float64_unsafe(
    pomelo_payload_t * payload,
    double * value
) {
    uint64_t temp;
    pomelo_payload_read_uint64_unsafe(payload, &temp);
    *value = *(double *) &temp;
}


void pomelo_payload_read_buffer_unsafe(
    pomelo_payload_t * payload,
    uint8_t * buffer,
    size_t size
) {
    assert(payload != NULL);
    assert(buffer != NULL);
    memcpy(buffer, payload->data + payload->position, size);
    payload->position += size;
}



size_t pomelo_payload_calc_packed_uint64_bytes(uint64_t value) {
    if (value & 0xFFFFFFFF00000000ULL) {
        // Need > 4 bytes
        if (value & 0xFFFF000000000000ULL) {
            // Need > 6 bytes
            if (value & 0xFF00000000000000ULL) {
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


int pomelo_payload_read_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t * value
) {
    assert(payload != NULL);
    assert(value != NULL);

    size_t remain = payload->capacity - payload->position;
    if (bytes > remain) {
        return -1; // Not enough space
    }

    *value = 0;
    uint8_t byte = 0;
    for (size_t i = 0; i < bytes; i++) {
        pomelo_payload_read_uint8_unsafe(payload, &byte);
        *value |= ((uint64_t) byte) << (i * 8);
    }

    return 0;
}


void pomelo_payload_read_packed_uint64_unsafe(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t * value
) {
    assert(payload != NULL);
    assert(value != NULL);

    *value = 0;
    uint8_t byte = 0;
    for (size_t i = 0; i < bytes; i++) {
        pomelo_payload_read_uint8_unsafe(payload, &byte);
        *value |= ((uint64_t) byte) << (i * 8);
    }
}


int pomelo_payload_write_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t value
) {
    assert(payload != NULL);
    size_t remain = payload->capacity - payload->position;
    if (bytes > remain) {
        return -1; // Not enough space
    }

    // sequence bytes
    for (size_t i = 0; i < bytes; ++i) {
        pomelo_payload_write_uint8_unsafe(payload, value & 0xFF);
        value >>= 8;
    }

    return 0;
}


void pomelo_payload_write_packed_uint64_unsafe(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t value
) {
    assert(payload != NULL);

    // sequence bytes
    for (size_t i = 0; i < bytes; ++i) {
        pomelo_payload_write_uint8_unsafe(payload, value & 0xFF);
        value >>= 8;
    }
}
