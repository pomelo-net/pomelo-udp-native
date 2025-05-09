#include <assert.h>
#include <string.h>
#include "clock.h"

#define SQR(value) ((value) * (value))

#define POMELO_TIME_CONDITION_RTT_VAR_HIGH   SQR(10000000LL)     // 10 ms
#define POMELO_TIME_CONDITION_RTT_VAR_MEDIUM SQR( 5000000LL)     // 5 ms
#define POMELO_TIME_CONDITION_RTT_VAR_LOW    SQR( 5000000LL)     // 5 ms

#define POMELO_TIME_HIGH_MIN_TIMES_OF_PING   20
#define POMELO_TIME_HIGH_THRESHOLD_RTT_VAR   SQR(5000000LL)      // 5ms

#define POMELO_TIME_HIGH_MIN_DELTA_OFFSET     5000000LL          // 5ms

#define POMELO_TIME_MEDIUM_THRESHOLD_RECENT_OFFSETS_VAR SQR(5000000LL) // 5ms
#define POMELO_TIME_MEDIUM_MIN_DELTA_OFFSET  10000000LL          // 10ms

#define POMELO_TIME_LOW_MIN_DELTA_MEAN_RECENT_OFFSETS 10000000LL // 10ms

#define calc_delta_offset(first, second) \
    ((first) > (second) ? ((first) - (second)) : ((second) - (first)))


void pomelo_delivery_clock_init(
    pomelo_delivery_clock_t * clock,
    pomelo_platform_t * platform
) {
    assert(clock != NULL);
    assert(platform != NULL);

    memset(clock, 0, sizeof(pomelo_delivery_clock_t));
    clock->platform = platform;

    // Initialize sample set
    pomelo_sample_set_i64_init(
        &clock->recent_offsets_sample,
        clock->recent_offsets,
        POMELO_PROTOCOL_CLOCK_RECENT_OFFSETS_SIZE
    );
}


void pomelo_delivery_clock_set(
    pomelo_delivery_clock_t * clock,
    uint64_t value
) {
    assert(clock != NULL);
    pomelo_atomic_int64_store(
        &clock->offset,
        value - pomelo_platform_hrtime(clock->platform)
    );
}


bool pomelo_delivery_clock_sync(
    pomelo_delivery_clock_t * clock,
    pomelo_rtt_calculator_t * rtt,
    uint64_t req_send_time,  // t0
    uint64_t req_recv_time,  // t1
    uint64_t res_send_time,  // t2
    uint64_t res_recv_time   // t3
) {
    assert(clock != NULL);
    assert(rtt != NULL);

    uint64_t rtt_var = 0;
    pomelo_rtt_calculator_get(rtt, NULL, &rtt_var);

    // offset = [(t1 - t0) + (t2 - t3)] / 2
    int64_t offset = ((int64_t) (
        (req_recv_time - req_send_time) +
        (res_send_time - res_recv_time)
    )) / 2LL;

    // Submit recent offset
    pomelo_sample_set_i64_submit(&clock->recent_offsets_sample, offset);

    switch (clock->level) {
        case POMELO_DELIVERY_CLOCK_SYNC_LEVEL_HIGH:
            return pomelo_delivery_time_sync_high(clock, rtt_var, offset);

        case POMELO_DELIVERY_CLOCK_SYNC_LEVEL_MEDIUM:
            return pomelo_delivery_time_sync_medium(clock, rtt_var, offset);

        case POMELO_DELIVERY_CLOCK_SYNC_LEVEL_LOW:
            return pomelo_delivery_time_sync_low(clock, rtt_var, offset);
        
        default:
            return false;
    }
}


bool pomelo_delivery_time_sync_high(
    pomelo_delivery_clock_t * clock,
    uint64_t rtt_var,
    int64_t offset
) {
    assert(clock != NULL);
    if (rtt_var > POMELO_TIME_CONDITION_RTT_VAR_HIGH) {
        return false;
    }

    if (clock->high_sync_count < POMELO_TIME_HIGH_MIN_TIMES_OF_PING) {
        clock->high_sync_count++;
    } else if (rtt_var < POMELO_TIME_HIGH_THRESHOLD_RTT_VAR) {
        // More stable, downgrade level
        clock->level = POMELO_DELIVERY_CLOCK_SYNC_LEVEL_MEDIUM;
    }

    int64_t clock_offset = pomelo_atomic_int64_load(&clock->offset);
    int64_t delta_offset = calc_delta_offset(offset, clock_offset);
    if (delta_offset > POMELO_TIME_HIGH_MIN_DELTA_OFFSET) {
        pomelo_atomic_int64_store(&clock->offset, offset);
        return true;
    }

    return false;
}


bool pomelo_delivery_time_sync_medium(
    pomelo_delivery_clock_t * clock,
    uint64_t rtt_var,
    int64_t offset
) {
    assert(clock != NULL);
    if (rtt_var > POMELO_TIME_CONDITION_RTT_VAR_MEDIUM) {
        return false;
    }

    int64_t variance = 0;
    pomelo_sample_set_i64_calc(&clock->recent_offsets_sample, NULL, &variance);
    if (variance < POMELO_TIME_MEDIUM_THRESHOLD_RECENT_OFFSETS_VAR) {
        clock->level = POMELO_DELIVERY_CLOCK_SYNC_LEVEL_LOW;
    }

    int64_t clock_offset = pomelo_atomic_int64_load(&clock->offset);
    int64_t delta_offset = calc_delta_offset(offset, clock_offset);
    if (delta_offset > POMELO_TIME_MEDIUM_MIN_DELTA_OFFSET) {
        pomelo_atomic_int64_store(&clock->offset, offset);
        return true;
    }

    return false;
}


bool pomelo_delivery_time_sync_low(
    pomelo_delivery_clock_t * clock,
    uint64_t rtt_var,
    int64_t offset
) {
    assert(clock != NULL);
    if (rtt_var > POMELO_TIME_CONDITION_RTT_VAR_LOW) {
        return false;
    }

    int64_t mean = 0;
    pomelo_sample_set_i64_calc(&clock->recent_offsets_sample, &mean, NULL);
    int64_t delta_offset = calc_delta_offset(mean, offset);
    if (delta_offset > POMELO_TIME_LOW_MIN_DELTA_MEAN_RECENT_OFFSETS) {
        pomelo_atomic_int64_store(&clock->offset, mean);
        return true;
    }

    return false;
}
