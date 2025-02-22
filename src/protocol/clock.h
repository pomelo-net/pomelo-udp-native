#ifndef POMELO_PROTOCOL_TIME_SRC_H
#define POMELO_PROTOCOL_TIME_SRC_H
#include <stdint.h>
#include <stdbool.h>
#include "pomelo/platform.h"
#include "base/rtt.h"
#include "utils/sampling.h"
#include "utils/atomic.h"
#ifdef __cplusplus
extern "C" {
#endif


/*
Time synchronization strategy:
- High: Client starts synchronizing clock with server
    + Duration: While VAR(rtt) >= SQR(5ms) and at least 20 times of ping
    + Resync condition:
        * VAR(rtt) < SQR(10ms) and,
        * DELTA(offset) > 5ms

- Medium: Client has just established synchronized state with server
    + Duration: While VAR(recent_offsets) >= SQR(5ms)
    + Resync condition:
        * VAR(rtt) < SQR(5ms) and,
        * DELTA(offset) > 10ms

- Low: Client clock is stable
    + Duration: Unlimited
    + Resync condition:
        * VAR(rtt) < SQR(5ms) and,
        * DELTA(MEAN(recent_offsets), offset) > 10ms
    + Sync with: MEAN(recent_offsets)
*/

#define POMELO_PROTOCOL_CLOCK_RECENT_OFFSETS_SIZE 10


/// @brief Synchronization level
typedef enum pomelo_protocol_clock_sync_level_e {
    POMELO_PROTOCOL_CLOCK_SYNC_LEVEL_HIGH,
    POMELO_PROTOCOL_CLOCK_SYNC_LEVEL_MEDIUM,
    POMELO_PROTOCOL_CLOCK_SYNC_LEVEL_LOW
} pomelo_protocol_clock_sync_level;



/// @brief Protocol clock
typedef struct pomelo_protocol_clock_s pomelo_protocol_clock_t;


struct pomelo_protocol_clock_s {
    /// @brief Offset
    pomelo_atomic_int64_t offset;

    /// @brief Platform
    pomelo_platform_t * platform;

    /// @brief Current synchronization level
    pomelo_protocol_clock_sync_level level;

    /* High level */
    int high_sync_count;

    /// @brief Recent offset sample
    pomelo_sample_set_i64_t recent_offsets_sample;

    /// @brief Recent offsets values
    int64_t recent_offsets[POMELO_PROTOCOL_CLOCK_RECENT_OFFSETS_SIZE];
};

/* -------------------------------------------------------------------------- */
/*                               Public APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Initialize protocol clock
void pomelo_protocol_clock_init(
    pomelo_protocol_clock_t * clock,
    pomelo_platform_t * platform
);


/// @brief Set clock time
void pomelo_protocol_clock_set(pomelo_protocol_clock_t * clock, uint64_t value);


/// @brief Synchronize time
bool pomelo_protocol_clock_sync(
    pomelo_protocol_clock_t * clock,
    pomelo_rtt_calculator_t * rtt,
    uint64_t req_send_time,  // t0
    uint64_t req_recv_time,  // t1
    uint64_t res_send_time,  // t2
    uint64_t res_recv_time   // t3
);


/* -------------------------------------------------------------------------- */
/*                              Private APIs                                  */
/* -------------------------------------------------------------------------- */

/// @brief Sync time in high level
bool pomelo_protocol_time_sync_high(
    pomelo_protocol_clock_t * time,
    uint64_t rtt_var,
    int64_t offset
);

/// @brief Sync time in medium level
bool pomelo_protocol_time_sync_medium(
    pomelo_protocol_clock_t * time,
    uint64_t rtt_var,
    int64_t offset
);

/// @brief Sync time in low level
bool pomelo_protocol_time_sync_low(
    pomelo_protocol_clock_t * time,
    uint64_t rtt_var,
    int64_t offset
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_PROTOCOL_TIME_SRC_H
