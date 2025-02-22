#ifndef POMELO_CODEC_SEQUENCE_SRC_H
#define POMELO_CODEC_SEQUENCE_SRC_H
#include "base/payload.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Calculate the number of bytes required to write packed uint64 number
/// @return The number of bytes required
size_t pomelo_codec_calc_packed_uint64_bytes(uint64_t value);


/// @brief Read packed uint64 value
int pomelo_codec_read_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t * value
);

/// @brief Write packed uint64 value
int pomelo_codec_write_packed_uint64(
    pomelo_payload_t * payload,
    size_t bytes,
    uint64_t value
);

/// @brief Calculate the number of bytes required to write packed int64 number
#define pomelo_codec_calc_packed_int64_bytes(value)                            \
    pomelo_codec_calc_packed_uint64_bytes((uint64_t) value)

/// @brief Read packed uint64 value
#define pomelo_codec_read_packed_int64(payload, bytes, value)                  \
    pomelo_codec_read_packed_uint64(payload, bytes, (uint64_t *) value)

/// @brief Write packed int64 value
#define pomelo_codec_write_packed_int64(payload, bytes, value)                 \
    pomelo_codec_write_packed_uint64(payload, bytes, (uint64_t) value)


#ifdef __cplusplus
}
#endif
#endif // POMELO_CODEC_SEQUENCE_SRC_H
