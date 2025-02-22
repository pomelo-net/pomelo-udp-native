#ifndef POMELO_UTILS_SRC_H
#define POMELO_UTILS_SRC_H

// Some common utiltities

/// Get maximum value of 2 numbers
#define POMELO_MAX(a, b) (((a) < (b)) ? b : a)

/// Get the minimum value of 2 numbers
#define POMELO_MIN(a, b) (((a) < (b)) ? a : b)

/// Convert time from nano seconds to miliseconds
#define POMELO_TIME_NS_TO_MS(ns) ((ns) / 1000000ULL)

/// Convert frequency to milliseconds
#define POMELO_FREQ_TO_MS(freq) (1000ULL / (freq))

/// Convert frequency to nanoseconds
#define POMELO_FREQ_TO_NS(freq) (1000000000ULL / (freq))

/// Convert seconds to milliseconds
#define POMELO_SECONDS_TO_MS(seconds) ((seconds) * 1000ULL)

/// Divide ceil
#define POMELO_CEIL_DIV(a, b) (((a) / (b)) + (((a) % (b)) > 0))

/// Check if the flag is set
#define POMELO_CHECK_FLAG(value, flag) ((value) & (flag))

/// Set the flag
#define POMELO_SET_FLAG(value, flag) ((value) |= (flag))

/// Unset a flag
#define POMELO_UNSET_FLAG(value, flag) ((value) &= ~(flag))

#endif // POMELO_UTILS_SRC_H
