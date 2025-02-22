#ifndef POMELO_CODEC_CHECKSUM_SRC_H
#define POMELO_CODEC_CHECKSUM_SRC_H
#include "base/packet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POMELO_CODEC_CHECKSUM_BYTES 32
#define POMELO_CHECKSUM_STATE_OPAQUE 384


/// The codec checksum state
typedef struct pomelo_codec_checksum_state_s {
    uint8_t opaque[POMELO_CHECKSUM_STATE_OPAQUE];
} pomelo_codec_checksum_state_t;


/// @brief Initailize the checksum
int pomelo_codec_checksum_init(pomelo_codec_checksum_state_t * state);


/// @brief Update the checksum.
/// This will use the capacity of payload as the length of buffer
int pomelo_codec_checksum_update(
    pomelo_codec_checksum_state_t * state,
    pomelo_payload_t * payload
);

/// @brief Finalize the checksum
int pomelo_codec_checksum_final(
    pomelo_codec_checksum_state_t * state,
    uint8_t * output
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_CODEC_CHECKSUM_SRC_H
