#ifndef POMELO_ADAPTER_INT_SRC_H
#define POMELO_ADAPTER_INT_SRC_H
#include "adapter/adapter.h"
#ifdef __cplusplus
extern "C" {
#endif


/*
    Default implementation of adapter.
    This adapter just forwards packets from UDP platform layer to
    protocol layer and vice vesa.
    This adapter prevents all unencrypted packets from delivering.
*/

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


void pomelo_adapter_alloc_callback(
    pomelo_adapter_t * adapter,
    pomelo_platform_iovec_t * iovec
);


/// @brief Receiving callback of adapter
void pomelo_adapter_recv_callback(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_platform_iovec_t * iovec,
    int status
);


/// @brief Start receiving packets from UDP socket
void pomelo_adapter_recv_start(pomelo_adapter_t * adapter);


#ifdef __cplusplus
}
#endif
#endif // POMELO_ADAPTER_INT_SRC_H
