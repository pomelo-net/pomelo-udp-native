#include <assert.h>
#include <string.h>
#include "rtt.h"


#ifdef POMELO_MULTI_THREAD
#define pomelo_rtt_value_get(rtt_value)            \
    pomelo_atomic_uint64_load(&(rtt_value))
#define pomelo_rtt_value_set(rtt_value, value)     \
    pomelo_atomic_uint64_store(&(rtt_value), value)

#else // !POMELO_MULTI_THREAD
#define pomelo_rtt_value_get(rtt_value) rtt_value
#define pomelo_rtt_value_set(rtt_value, value) (rtt_value) = value

#endif // POMELO_MULTI_THREAD


void pomelo_rtt_calculator_init(pomelo_rtt_calculator_t * rtt) {
    assert(rtt != NULL);
    memset(rtt, 0, sizeof(pomelo_rtt_calculator_t));
    pomelo_rtt_value_set(rtt->mean, 0);
    pomelo_rtt_value_set(rtt->variance, 0);

    // Initialize sample set
    pomelo_sample_set_u64_init(
        &rtt->sample,
        rtt->sample_values,
        POMELO_RTT_SAMPLE_SET_SIZE
    );
}


void pomelo_rtt_calculator_get(
    pomelo_rtt_calculator_t * rtt,
    uint64_t * mean,
    uint64_t * variance
) {
    assert(rtt != NULL);
    if (mean) {
        *mean = pomelo_rtt_value_get(rtt->mean);
    }

    if (variance) {
        *variance = pomelo_rtt_value_get(rtt->variance);
    }
}


pomelo_rtt_entry_t * pomelo_rtt_calculator_next_entry(
    pomelo_rtt_calculator_t * rtt,
    uint64_t time
) {
    assert(rtt != NULL);

    uint64_t sequence = rtt->entry_sequence++;
    if (rtt->entry_sequence > POMELO_RTT_MAX_SEQUENCE) {
        rtt->entry_sequence = 0; // Reset sequence number
    }

    uint64_t index = sequence % POMELO_RTT_ENTRY_BUFFER_SIZE;
    pomelo_rtt_entry_t * entry = rtt->entries + index;

    entry->valid = true;
    entry->sequence = sequence;
    entry->time = time;

    return entry;
}


void pomelo_rtt_calculator_submit_entry(
    pomelo_rtt_calculator_t * rtt,
    pomelo_rtt_entry_t * entry,
    uint64_t recv_time,
    uint64_t reply_delta_time
) {
    assert(rtt != NULL);
    assert(entry != NULL);

    entry->valid = false;

    // Update sample
    uint64_t value = recv_time - entry->time;
    if (value < reply_delta_time) {
        return; // Invalid, discard
    }

    pomelo_sample_set_u64_submit(&rtt->sample, value - reply_delta_time);
    
    // Update mean & variance
    uint64_t mean = 0;
    uint64_t variance = 0;
    pomelo_sample_set_u64_calc(&rtt->sample, &mean, &variance);

    pomelo_rtt_value_set(rtt->mean, mean);
    pomelo_rtt_value_set(rtt->variance, variance);
}


pomelo_rtt_entry_t * pomelo_rtt_calculator_entry(
    pomelo_rtt_calculator_t * rtt,
    uint64_t sequence
) {
    assert(rtt != NULL);
    uint64_t entry_index = sequence % POMELO_RTT_ENTRY_BUFFER_SIZE;
    pomelo_rtt_entry_t * entry = rtt->entries + entry_index;
    if (!entry->valid || entry->sequence != sequence) {
        return NULL; // Not match, discard
    }
    return entry;
}
