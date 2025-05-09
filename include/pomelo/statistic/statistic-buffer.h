#ifndef POMELO_STATISTIC_BUFFER_H
#define POMELO_STATISTIC_BUFFER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief The statistic of data context module
typedef struct pomelo_statistic_buffer_s pomelo_statistic_buffer_t;

struct pomelo_statistic_buffer_s {
    /// @brief The number of active buffers
    size_t buffers;
};

#ifdef __cplusplus
}
#endif

#endif // POMELO_STATISTIC_BUFFER_H
