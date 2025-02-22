#ifndef POMELO_STATISTIC_API_H
#define POMELO_STATISTIC_API_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


struct pomelo_statistic_api_s {
    /// @brief The number of in-use api messages
    size_t messages;

    /// @brief The number of in-use api sessions
    size_t sessions;
};


/// @brief The statistic of API module
typedef struct pomelo_statistic_api_s pomelo_statistic_api_t;


#ifdef __cplusplus
}
#endif
#endif // POMELO_STATISTIC_API_H
