#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "payload.h"


/* -------------------------------------------------------------------------- */
/*                               Public API                                   */
/* -------------------------------------------------------------------------- */

int pomelo_payload_write_uint8(pomelo_payload_t * payload, uint8_t value) {
    assert(payload);

    if (payload->position + 1 > payload->capacity) {
        return -1;
    }

    payload->data[payload->position++] = value;
    return 0;
}


int pomelo_payload_write_uint16(pomelo_payload_t * payload, uint16_t value) {
    assert(payload);

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
    assert(payload);

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
    assert(payload);

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
    // Assume that floating point and integer numbers have the same endianness
    return pomelo_payload_write_uint32(payload, *((uint32_t *) &value));
}


int pomelo_payload_write_float64(pomelo_payload_t * payload, double value) {
    // Assume that floating point and integer numbers have the same endianness
    return pomelo_payload_write_uint64(payload, *((uint64_t *) &value));
}


int pomelo_payload_write_buffer(
    pomelo_payload_t * payload,
    const uint8_t * buffer,
    size_t size
) {
    assert(payload);
    assert(buffer);

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
    assert(payload);

    if (pad_size > payload->capacity) {
        return -1;
    }

    size_t position = payload->position;
    if (pad_size <= position) {
        return 0;
    }

    size_t remain = pad_size - position;
    memset(payload->data + position, 0, remain);
    payload->position = pad_size;
    return 0;
}


int pomelo_payload_read_uint8(pomelo_payload_t * payload, uint8_t * value) {
    assert(payload);
    assert(value);

    if (payload->position + 1 > payload->capacity) {
        return -1;
    }

    *value = payload->data[payload->position++];
    return 0;
}


int pomelo_payload_read_uint16(pomelo_payload_t * payload, uint16_t * value) {
    assert(payload);
    assert(value);

    size_t position = payload->position;
    if (position + 2 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;
    *value = data[position] | data[position + 1] << 8;
    payload->position += 2;
    return 0;
}


int pomelo_payload_read_uint32(pomelo_payload_t * payload, uint32_t * value) {
    assert(payload);
    assert(value);

    size_t position = payload->position;
    if (position + 4 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;

    *value = (
        data[position] |
        data[position + 1] << 8 |
        data[position + 2] << 16 |
        data[position + 3] << 24
    );

    payload->position += 4;
    return 0;
}


int pomelo_payload_read_uint64(pomelo_payload_t * payload, uint64_t * value) {
    assert(payload);
    assert(value);

    size_t position = payload->position;
    if (position + 8 > payload->capacity) {
        return -1;
    }

    uint8_t * data = payload->data;

    *value = (
        (uint64_t) data[position] |
        (uint64_t) data[position + 1] << 8 |
        (uint64_t) data[position + 2] << 16 |
        (uint64_t) data[position + 3] << 24 |
        (uint64_t) data[position + 4] << 32 |
        (uint64_t) data[position + 5] << 40 |
        (uint64_t) data[position + 6] << 48 |
        (uint64_t) data[position + 7] << 56
    );

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
    // Assume that floating point and integer numbers have the same endianness
    return pomelo_payload_read_uint32(payload, (uint32_t *) value);
}


int pomelo_payload_read_float64(pomelo_payload_t * payload, double * value) {
    // Assume that floating point and integer numbers have the same endianness
    return pomelo_payload_read_uint64(payload, (uint64_t *) value);
}


int pomelo_payload_read_buffer(
    pomelo_payload_t * payload,
    uint8_t * buffer,
    size_t size
) {
    assert(payload);
    assert(buffer);

    if (payload->position + size > payload->capacity) {
        return -1;
    }
    memcpy(buffer, payload->data + payload->position, size);
    payload->position += size;
    return 0;
}
