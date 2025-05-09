#ifndef POMELO_PAYLOAD_SRC_H
#define POMELO_PAYLOAD_SRC_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_payload_s {
    /// @brief The capacity of payload
    size_t capacity;
    
    /// @brief The current position of reading or writing
    size_t position;
    
    /// @brief Data of payload
    uint8_t * data;
};

/// View of an uint8 array
typedef struct pomelo_payload_s pomelo_payload_t;


/* 
    Write value to payload.
    payload: The output payload
    value: The writing value
    bits: The number of bits to write [0-64]
    Return 0 on success, or an error code < 0 on failure.
*/
int pomelo_payload_write_uint8(pomelo_payload_t * payload, uint8_t value);
int pomelo_payload_write_uint16(pomelo_payload_t * payload, uint16_t value);
int pomelo_payload_write_uint32(pomelo_payload_t * payload, uint32_t value);
int pomelo_payload_write_uint64(pomelo_payload_t * payload, uint64_t value);
int pomelo_payload_write_int8(pomelo_payload_t * payload, int8_t value);
int pomelo_payload_write_int16(pomelo_payload_t * payload, int16_t value);
int pomelo_payload_write_int32(pomelo_payload_t * payload, int32_t value);
int pomelo_payload_write_int64(pomelo_payload_t * payload, int64_t value);
int pomelo_payload_write_float32(pomelo_payload_t * payload, float value);
int pomelo_payload_write_float64(pomelo_payload_t * payload, double value);
int pomelo_payload_write_buffer(
    pomelo_payload_t * payload,
    const uint8_t * buffer,
    size_t size
);

/// @brief Zero pad the payload to specific padding size
int pomelo_payload_zero_pad(pomelo_payload_t * payload, size_t pad_size);

/* 
    Read value from payload.
    payload: The input payload
    value: The output value
    bits: The number of bits to read [0-64]
    Return 0 on success, or an error code < 0 on failure.
*/
int pomelo_payload_read_uint8(pomelo_payload_t * payload, uint8_t * value);
int pomelo_payload_read_uint16(pomelo_payload_t * payload, uint16_t * value);
int pomelo_payload_read_uint32(pomelo_payload_t * payload, uint32_t * value);
int pomelo_payload_read_uint64(pomelo_payload_t * payload, uint64_t * value);
int pomelo_payload_read_int8(pomelo_payload_t * payload, int8_t * value);
int pomelo_payload_read_int16(pomelo_payload_t * payload, int16_t * value);
int pomelo_payload_read_int32(pomelo_payload_t * payload, int32_t * value);
int pomelo_payload_read_int64(pomelo_payload_t * payload, int64_t * value);
int pomelo_payload_read_float32(pomelo_payload_t * payload, float * value);
int pomelo_payload_read_float64(pomelo_payload_t * payload, double * value);
int pomelo_payload_read_buffer(
    pomelo_payload_t * payload,
    uint8_t * buffer,
    size_t size
);

/// @brief Get the remain bytes of payload
#define pomelo_payload_remain(payload)                                         \
    ((payload)->capacity - (payload)->position)


/* -------------------------------------------------------------------------- */
/*                           Unsafe functions                                 */
/* Following functions are unsafe, because they do not check the bounds of    */
/* payload.                                                                   */
/* -------------------------------------------------------------------------- */

void pomelo_payload_write_uint8_unsafe(
    pomelo_payload_t * payload,
    uint8_t value
);

void pomelo_payload_write_uint16_unsafe(
    pomelo_payload_t * payload,
    uint16_t value
);

void pomelo_payload_write_uint32_unsafe(
    pomelo_payload_t * payload,
    uint32_t value
);

void pomelo_payload_write_uint64_unsafe(
    pomelo_payload_t * payload,
    uint64_t value
);

void pomelo_payload_write_int8_unsafe(
    pomelo_payload_t * payload,
    int8_t value
);

void pomelo_payload_write_int16_unsafe(
    pomelo_payload_t * payload,
    int16_t value
);

void pomelo_payload_write_int32_unsafe(
    pomelo_payload_t * payload,
    int32_t value
);

void pomelo_payload_write_int64_unsafe(
    pomelo_payload_t * payload,
    int64_t value
);

void pomelo_payload_write_float32_unsafe(
    pomelo_payload_t * payload,
    float value
);

void pomelo_payload_write_float64_unsafe(
    pomelo_payload_t * payload,
    double value
);

void pomelo_payload_write_buffer_unsafe(
    pomelo_payload_t * payload,
    const uint8_t * buffer,
    size_t size
);

void pomelo_payload_zero_pad_unsafe(
    pomelo_payload_t * payload,
    size_t pad_size
);

void pomelo_payload_read_uint8_unsafe(
    pomelo_payload_t * payload,
    uint8_t * value
);

void pomelo_payload_read_uint16_unsafe(
    pomelo_payload_t * payload,
    uint16_t * value
);

void pomelo_payload_read_uint32_unsafe(
    pomelo_payload_t * payload,
    uint32_t * value
);

void pomelo_payload_read_uint64_unsafe(
    pomelo_payload_t * payload,
    uint64_t * value
);

void pomelo_payload_read_int8_unsafe(
    pomelo_payload_t * payload,
    int8_t * value
);

void pomelo_payload_read_int16_unsafe(
    pomelo_payload_t * payload,
    int16_t * value
);

void pomelo_payload_read_int32_unsafe(
    pomelo_payload_t * payload,
    int32_t * value
);

void pomelo_payload_read_int64_unsafe(
    pomelo_payload_t * payload,
    int64_t * value
);

void pomelo_payload_read_float32_unsafe(
    pomelo_payload_t * payload,
    float * value
);

void pomelo_payload_read_float64_unsafe(
    pomelo_payload_t * payload,
    double * value
);

void pomelo_payload_read_buffer_unsafe(
    pomelo_payload_t * payload,
    uint8_t * buffer,
    size_t size
);

/* -------------------------------------------------------------------------- */
/*                           Packed number APIs                               */
/* -------------------------------------------------------------------------- */


/// @brief Calculate the number of bytes required to write packed uint64 number
/// @return The number of bytes required
size_t pomelo_payload_calc_packed_uint64_bytes(uint64_t value);


/// @brief Read packed uint64 value
int pomelo_payload_read_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t * value
);


/// @brief Read packed uint64 value without checking the payload
void pomelo_payload_read_packed_uint64_unsafe(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t * value
);


/// @brief Write packed uint64 value
int pomelo_payload_write_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t value
);


/// @brief Write packed uint64 value without checking the payload
void pomelo_payload_write_packed_uint64_unsafe(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t value
);


/// @brief Calculate the number of bytes required to write packed int64 number
#define pomelo_payload_calc_packed_int64_bytes(value)                          \
    pomelo_payload_calc_packed_uint64_bytes((uint64_t) value)


/// @brief Read packed uint64 value without checking the payload
#define pomelo_payload_read_packed_int64_unsafe(payload, bytes, value)         \
    pomelo_payload_read_packed_uint64_unsafe(payload, bytes, (uint64_t *) value)


/// @brief Write packed int64 value without checking the payload
#define pomelo_payload_write_packed_int64_unsafe(payload, bytes, value)        \
    pomelo_payload_write_packed_uint64_unsafe(payload, bytes, (uint64_t) value)


#ifdef __cplusplus
}
#endif
#endif // POMELO_PAYLOAD_SRC_H
