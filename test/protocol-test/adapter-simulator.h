#ifndef POMELO_TEST_PROTOCOL_ADAPTER_SIMULATOR_H
#define POMELO_TEST_PROTOCOL_ADAPTER_SIMULATOR_H
#include "adapter/adapter.h"
#ifdef __cplusplus
extern "C" {
#endif


typedef void (*pomelo_adapter_simulator_send_handler)(
    pomelo_address_t * address,
    pomelo_buffer_view_t * view
);


struct pomelo_adapter_s {
    /// @brief Extra data of adater
    void * extra;

    /// @brief Allocator
    pomelo_allocator_t * allocator;

    /// @brief Send callback
    pomelo_adapter_simulator_send_handler send_handler;
};


/// @brief Dispatch receiving a message
void pomelo_adapter_recv(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_TEST_PROTOCOL_ADAPTER_SIMULATOR_H
