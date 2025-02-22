#include <assert.h>
#include <string.h>
#include "sampling.h"


void pomelo_sample_set_u64_init(
    pomelo_sample_set_u64_t * sample_set,
    uint64_t * values,
    size_t size
) {
    assert(sample_set != NULL);
    assert(values != NULL);
    assert(size > 0);
    
    memset(sample_set, 0, sizeof(pomelo_sample_set_u64_t));
    sample_set->values = values;
    sample_set->size = size;
}


void pomelo_sample_set_u64_submit(
    pomelo_sample_set_u64_t * sample_set,
    uint64_t value
) {
    assert(sample_set != NULL);

    if (!sample_set->initialized) {
        // This is the first received value
        for (size_t i = 0; i < sample_set->size; i++) {
            sample_set->values[i] = value;
        }

        sample_set->sum = value * sample_set->size;
        sample_set->sum_squared = sample_set->sum * value;
        sample_set->initialized = true;
        return;
    }

    // Update sums
    uint64_t prev_value = sample_set->values[sample_set->index];
    sample_set->sum = sample_set->sum + value - prev_value;
    sample_set->sum_squared =
        sample_set->sum_squared + value * value - prev_value * prev_value;

    // Update the value and increase the index
    sample_set->values[sample_set->index] = value;
    sample_set->index = (sample_set->index + 1) % sample_set->size;
}


void pomelo_sample_set_u64_calc(
    pomelo_sample_set_u64_t * sample_set,
    uint64_t * mean,
    uint64_t * variance
) {
    assert(sample_set != NULL);

    // Update mean & variance
    uint64_t mean_value = sample_set->sum / sample_set->size;
    if (mean) {
        *mean = mean_value;
    }

    if (variance) {
        *variance = ( // VAR(X) = E(X^2) - E(X)^2
            sample_set->sum_squared / sample_set->size - mean_value * mean_value
        );
    }
}


void pomelo_sample_set_i64_init(
    pomelo_sample_set_i64_t * sample_set,
    int64_t * values,
    size_t size
) {
    assert(sample_set != NULL);
    assert(values != NULL);
    assert(size > 0);
    
    memset(sample_set, 0, sizeof(pomelo_sample_set_u64_t));
    sample_set->values = values;
    sample_set->size = size;
}


void pomelo_sample_set_i64_submit(
    pomelo_sample_set_i64_t * sample_set,
    int64_t value
) {
    assert(sample_set != NULL);

    if (!sample_set->initialized) {
        // This is the first received value
        for (size_t i = 0; i < sample_set->size; i++) {
            sample_set->values[i] = value;
        }

        sample_set->sum = value * sample_set->size;
        sample_set->sum_squared = sample_set->sum * value;
        sample_set->initialized = true;
        return;
    }

    // Update sums
    uint64_t prev_value = sample_set->values[sample_set->index];
    sample_set->sum = sample_set->sum + value - prev_value;
    sample_set->sum_squared =
        sample_set->sum_squared + value * value - prev_value * prev_value;

    // Update the value and increase the index
    sample_set->values[sample_set->index] = value;
    sample_set->index = (sample_set->index + 1) % sample_set->size;
}


void pomelo_sample_set_i64_calc(
    pomelo_sample_set_i64_t * sample_set,
    int64_t * mean,
    int64_t * variance
) {
    assert(sample_set != NULL);

    // Update mean & variance
    uint64_t mean_value = sample_set->sum / sample_set->size;
    if (mean) {
        *mean = mean_value;
    }

    if (variance) {
        *variance = ( // VAR(X) = E(X^2) - E(X)^2
            sample_set->sum_squared / sample_set->size - mean_value * mean_value
        );
    }
}
