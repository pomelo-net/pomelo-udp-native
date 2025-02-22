#include <assert.h>
#include <string.h>
#include "default.h"



pomelo_adapter_t * pomelo_adapter_create(pomelo_adapter_options_t * options) {
    assert(options != NULL);
    if (!options->platform) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_adapter_t * adapter =
        pomelo_allocator_malloc_t(allocator, pomelo_adapter_t);
    if (!adapter) {
        return NULL;
    }
    memset(adapter, 0, sizeof(pomelo_adapter_t));

    adapter->allocator = allocator;
    adapter->platform = options->platform;

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


uint8_t pomelo_adapter_get_capability(pomelo_adapter_t * adapter) {
    (void) adapter;

    // Default adapter only supports encrypted packets
    return POMELO_ADAPTER_CAPABILITY_ENCRYPTED;
}


int pomelo_adapter_connect(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address
) {
    assert(adapter != NULL);
    assert(address != NULL);

    adapter->udp = pomelo_platform_udp_connect(adapter->platform, address);
    if (!adapter->udp) {
        return -1; // Failed to start connecting
    }

    pomelo_adapter_init_callbacks(adapter);
    return 0;
}


int pomelo_adapter_listen(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address
) {
    assert(adapter != NULL);
    assert(address != NULL);

    adapter->udp = pomelo_platform_udp_bind(adapter->platform, address);
    if (!adapter->udp) {
        return -1; // Failed to start listening
    }

    pomelo_adapter_init_callbacks(adapter);
    return 0;
}


int pomelo_adapter_stop(pomelo_adapter_t * adapter) {
    assert(adapter != NULL);
    if (!adapter->udp) {
        return -1;
    }
    return pomelo_platform_udp_stop(adapter->platform, adapter->udp);
}


int pomelo_adapter_send(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_packet_t * packet,
    void * callback_data,
    bool encrypted
) {
    assert(adapter != NULL);
    assert(packet != NULL);
    if (!encrypted) {
        return -1; // Default adapter requires all packets must be encrypted.
    }

    pomelo_buffer_vector_t buffers[2]; // Header & body
    buffers[0].data = packet->header.data;
    buffers[0].length = packet->header.position;
    buffers[1].data = packet->body.data;
    buffers[1].length = packet->body.position;

    return pomelo_platform_udp_send(
        adapter->platform,
        adapter->udp,
        address,
        sizeof(buffers) / sizeof(buffers[0]),
        buffers,
        callback_data
    );
}


void pomelo_adapter_recv_callback(
    pomelo_adapter_t * adapter,
    pomelo_address_t * address,
    pomelo_buffer_vector_t * buffer_vector,
    int status
) {
    pomelo_adapter_on_recv(adapter, address, buffer_vector, status, true);
}


void pomelo_adapter_init_callbacks(pomelo_adapter_t * adapter) {
    assert(adapter != NULL);
    pomelo_platform_udp_set_callbacks(
        adapter->platform,
        adapter->udp,
        adapter, // context
        (pomelo_platform_alloc_cb) pomelo_adapter_on_alloc,
        (pomelo_platform_recv_cb) pomelo_adapter_recv_callback,
        (pomelo_platform_send_cb) pomelo_adapter_on_sent
    );
}
