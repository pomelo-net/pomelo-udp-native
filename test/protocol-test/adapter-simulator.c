#include <assert.h>
#include <string.h>
#include "adapter-simulator.h"


pomelo_adapter_t * pomelo_adapter_create(pomelo_adapter_options_t * options) {
    assert(options != NULL);

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_adapter_t * adapter =
        pomelo_allocator_malloc_t(allocator, pomelo_adapter_t);
    if (!adapter) return NULL;
    memset(adapter, 0, sizeof(pomelo_adapter_t));

    adapter->allocator = allocator;

    return adapter;
}


void pomelo_adapter_destroy(pomelo_adapter_t * adapter) {
    assert(adapter != NULL);
    pomelo_allocator_free(adapter->allocator, adapter);
}


void pomelo_adapter_set_extra(pomelo_adapter_t * adapter, void * extra) {
    assert(adapter != NULL);
    adapter->extra = extra;
}


void * pomelo_adapter_get_extra(pomelo_adapter_t * adapter) {
    assert(adapter != NULL);
    return adapter->extra;
}


uint32_t pomelo_adapter_get_capability(pomelo_adapter_t * adapter) {
    (void) adapter;
    // This adapter only supports encrypted packets
    return (
        POMELO_ADAPTER_CAPABILITY_SERVER_ENCRYPTED |
        POMELO_ADAPTER_CAPABILITY_CLIENT_ENCRYPTED
    );
}


int pomelo_adapter_connect(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address
) {
    (void) adapter;
    (void) address;
    return 0; // Ignore
}


int pomelo_adapter_listen(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address
) {
    (void) adapter;
    (void) address;
    return 0; // Ignore
}


int pomelo_adapter_stop(pomelo_adapter_t * adapter) {
    (void) adapter;
    return 0; // Ignore
}


int pomelo_adapter_send(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view,
    bool encrypted
) {
    assert(adapter != NULL);
    if (!encrypted) {
        return -1;
    }

    // Dispatch incoming message
    if (adapter->send_handler) {
        adapter->send_handler(address, view);
    }

    return 0;
}


void pomelo_adapter_recv(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_view_t * view
) {
    pomelo_adapter_on_recv(adapter, address, view, true);
}
