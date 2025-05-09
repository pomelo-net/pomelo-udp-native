#ifndef POMELO_STATISTIC_API_H
#define POMELO_STATISTIC_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief The statistic of API module
typedef struct pomelo_statistic_api_s pomelo_statistic_api_t;

struct pomelo_statistic_api_s {
    /// @brief The number of active messages
    size_t messages;

    /// @brief The number of active builtin sessions
    size_t builtin_sessions;

    /// @brief The number of active plugin sessions
    size_t plugin_sessions;

    /// @brief The number of active builtin channels
    size_t builtin_channels;

    /// @brief The number of active plugin channels
    size_t plugin_channels;
};

#ifdef __cplusplus
}
#endif

#endif // POMELO_STATISTIC_API_H
