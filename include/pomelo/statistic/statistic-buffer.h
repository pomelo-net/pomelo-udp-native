#ifndef POMELO_STATISTIC_BUFFER_H
#define POMELO_STATISTIC_BUFFER_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_statistic_buffer_context_s {
    /// @brief The number of in-use buffers
    size_t buffers;
};


/// @brief The statistic of data context module
typedef struct pomelo_statistic_buffer_context_s
    pomelo_statistic_buffer_context_t;


#ifdef __cplusplus
}
#endif
#endif // POMELO_STATISTIC_DATA_CONTEXT_H
