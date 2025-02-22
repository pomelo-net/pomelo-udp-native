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


/// @brief Receiving callback of adapter
void pomelo_adapter_recv_callback(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_vector_t * buffer_vector,
    int status
);


/// @brief Initialize callbacks
void pomelo_adapter_init_callbacks(pomelo_adapter_t * adapter);


#ifdef __cplusplus
}
#endif
#endif // POMELO_ADAPTER_INT_SRC_H
