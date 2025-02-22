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
#define pomelo_payload_remain(payload) (payload->capacity - payload->position)

/// @brief Pack the payload.
/// This will make the "writing payload" becomes "reading payload"
#define pomelo_payload_pack(payload)                                           \
do {                                                                           \
    (payload)->capacity = (payload)->position;                                 \
    (payload)->position = 0;                                                   \
} while (0)


#ifdef __cplusplus
}
#endif
#endif // POMELO_PAYLOAD_SRC_H
