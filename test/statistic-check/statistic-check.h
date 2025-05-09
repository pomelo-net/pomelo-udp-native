#ifndef POMELO_TEST_STATISTIC_CHECK_H
#define POMELO_TEST_STATISTIC_CHECK_H
#include "pomelo/statistic.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Check resource leak
void pomelo_statistic_check_resource_leak(pomelo_statistic_t * statistic);


/// @brief Check resource leak of protocol
void pomelo_statistic_protocol_check_resource_leak(
    pomelo_statistic_protocol_t * protocol
);


/// @brief Check resource leak of delivery
void pomelo_statistic_delivery_check_resource_leak(
    pomelo_statistic_delivery_t * delivery
);


/// @brief Check resource leak of API
void pomelo_statistic_api_check_resource_leak(
    pomelo_statistic_api_t * api
);


/// @brief Check resource leak of buffer
void pomelo_statistic_buffer_check_resource_leak(
    pomelo_statistic_buffer_t * buffer
);

#ifdef __cplusplus
}
#endif
#endif // POMELO_TEST_STATISTIC_CHECK_H
