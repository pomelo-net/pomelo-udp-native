#ifndef POMELO_ADAPTER_SRC_H
#define POMELO_ADAPTER_SRC_H
#include "pomelo/allocator.h"
#include "pomelo/common.h"
#include "platform/platform.h"
#include "base/buffer.h"
#ifdef __cplusplus
extern "C" {
#endif


/**
 * This is the middle layer between protocol and platform.
 * It forwards packets from UDP platform layer to protocol layer and
 * vice versa.
 */

/// Adapter supports transmiting unencrypted packets for server
#define POMELO_ADAPTER_CAPABILITY_SERVER_UNENCRYPTED (1 << 0)

/// Adapter supports transmiting encrypted packets for server
#define POMELO_ADAPTER_CAPABILITY_SERVER_ENCRYPTED   (1 << 1)

/// Adapter supports both unencrypted and encrypted packets for server
#define POMELO_ADAPTER_CAPABILITY_SERVER_ALL                                   \
    (POMELO_ADAPTER_CAPABILITY_SERVER_UNENCRYPTED |                            \
    POMELO_ADAPTER_CAPABILITY_SERVER_ENCRYPTED)

/// Adapter supports transmiting unencrypted packets for client
#define POMELO_ADAPTER_CAPABILITY_CLIENT_UNENCRYPTED (1 << 2)

/// Adapter supports transmiting encrypted packets for client
#define POMELO_ADAPTER_CAPABILITY_CLIENT_ENCRYPTED   (1 << 3)

/// Adapter supports both unencrypted and encrypted packets for client
#define POMELO_ADAPTER_CAPABILITY_CLIENT_ALL                                   \
    (POMELO_ADAPTER_CAPABILITY_CLIENT_UNENCRYPTED |                            \
    POMELO_ADAPTER_CAPABILITY_CLIENT_ENCRYPTED)


/// @brief Creating options of adapter
typedef struct pomelo_adapter_options_s pomelo_adapter_options_t;


struct pomelo_adapter_options_s {
    /// @brief Allocator of adapter
    pomelo_allocator_t * allocator;

    /// @brief Platform of adapter
    pomelo_platform_t * platform;
};


/// @brief Create the adapter
pomelo_adapter_t * pomelo_adapter_create(pomelo_adapter_options_t * options);


/// @brief Destroy the adapter
void pomelo_adapter_destroy(pomelo_adapter_t * adapter);


/// @brief Set extra data
void pomelo_adapter_set_extra(pomelo_adapter_t * adapter, void * extra);


/// @brief Get extra data
void * pomelo_adapter_get_extra(pomelo_adapter_t * adapter);


/// @brief Get the capability of adapter. Protocol layer relies on returned
/// value of this function to make decision whether encrypting packets or not.
/// If adapter supports both unencrypted and encrypted packets, protocol will
/// use encrypting.
uint32_t pomelo_adapter_get_capability(pomelo_adapter_t * adapter);


/// @brief Start adapter as client and connect to server
int pomelo_adapter_connect(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address
);


/// @brief Start adapter as server and listen on address
int pomelo_adapter_listen(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address
);


/// @brief Stop adapter
int pomelo_adapter_stop(pomelo_adapter_t * adapter);


/// @brief Send a packet
int pomelo_adapter_send(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
);


/// @brief Process when received a payload
void pomelo_adapter_on_recv(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
);


/// @brief Allocate a buffer for receiving
pomelo_buffer_t * pomelo_adapter_buffer_acquire(pomelo_adapter_t * adapter);


#ifdef __cplusplus
}
#endif
#endif // POMELO_ADAPTER_SRC_H
