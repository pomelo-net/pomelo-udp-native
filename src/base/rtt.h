#ifndef POMELO_BASE_RTT_H
#define POMELO_BASE_RTT_H
#include <stdbool.h>
#include <stdint.h>
#ifdef POMELO_MULTI_THREAD
#include "utils/atomic.h"
#endif // POMELO_MULTI_THREAD
#include "utils/sampling.h"


#ifdef __cplusplus
extern "C" {
#endif

/// Maximum number of elements in sample set
#define POMELO_RTT_SAMPLE_SET_SIZE 10

/// Maximum number of entries in sent pings
#define POMELO_RTT_ENTRY_BUFFER_SIZE 20

/// Max number of sequence value
#define POMELO_RTT_MAX_SEQUENCE 0xFFFF


/// @brief Round trip time calculator
typedef struct pomelo_rtt_calculator_s pomelo_rtt_calculator_t;

/// @brief A ping entry
typedef struct pomelo_rtt_entry_s pomelo_rtt_entry_t;

/// @brief Sample set
typedef struct pomelo_rtt_sample_s pomelo_rtt_sample_t;

#ifdef POMELO_MULTI_THREAD
typedef pomelo_atomic_uint64_t pomelo_rtt_value_t;
#else
typedef uint64_t pomelo_rtt_value_t;
#endif // POMELO_MULTI_THREAD


struct pomelo_rtt_entry_s {
    /// @brief Sent time
    uint64_t time;

    /// @brief Valid flags
    bool valid;

    /// @brief Sequence number of this entry
    uint64_t sequence;
};


struct pomelo_rtt_calculator_s {
    /// @brief Mean of RTT
    pomelo_rtt_value_t mean;

    /// @brief Variance of RTT
    pomelo_rtt_value_t variance;

    /// @brief Current sequence of the lastest entry
    uint64_t entry_sequence;

    /// @brief Entries set
    pomelo_rtt_entry_t entries[POMELO_RTT_ENTRY_BUFFER_SIZE];

    /// @brief Sample set
    pomelo_sample_set_u64_t sample;

    /// @brief Values for sample set
    uint64_t sample_values[POMELO_RTT_SAMPLE_SET_SIZE];
};

/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Initialize RTT
void pomelo_rtt_calculator_init(pomelo_rtt_calculator_t * rtt);

/// @brief Get the RTT values
void pomelo_rtt_calculator_get(
    pomelo_rtt_calculator_t * rtt,
    uint64_t * mean,
    uint64_t * variance
);

/// @brief Generate next outgoing entry
pomelo_rtt_entry_t * pomelo_rtt_calculator_next_entry(
    pomelo_rtt_calculator_t * rtt,
    uint64_t time
);

/// @brief Submit a received entry
void pomelo_rtt_calculator_submit_entry(
    pomelo_rtt_calculator_t * rtt,
    pomelo_rtt_entry_t *,
    uint64_t recv_time,
    uint64_t reply_delta_time
);

/// @brief Get the entry with specific sequence
pomelo_rtt_entry_t * pomelo_rtt_calculator_entry(
    pomelo_rtt_calculator_t * rtt,
    uint64_t sequence
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_BASE_RTT_H
