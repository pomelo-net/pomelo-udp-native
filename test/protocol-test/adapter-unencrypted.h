#ifndef POMELO_TEST_PROTOCOL_ADAPTER_UNENCRYPTED_H
#define POMELO_TEST_PROTOCOL_ADAPTER_UNENCRYPTED_H
#include "adapter/adapter.h"
#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------------------------------------------------------- */
/*                       Unencrypted adapter implementation                   */
/* -------------------------------------------------------------------------- */

struct pomelo_adapter_s {
    /// @brief Extra data of adater
    void * extra;

    /// @brief Allocator
    pomelo_allocator_t * allocator;
    
    /// @brief Platform
    pomelo_platform_t * platform;

    /// @brief Current UDP socket
    pomelo_platform_udp_t * udp;
};


#ifdef __cplusplus
}
#endif
#endif // POMELO_TEST_PROTOCOL_ADAPTER_UNENCRYPTED_H
