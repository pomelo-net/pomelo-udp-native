#include <assert.h>
#include <string.h>
#include "transporter.h"
#include "endpoint.h"
#include "bus.h"


int pomelo_delivery_endpoint_init(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_delivery_transporter_t * transporter
) {
    assert(endpoint != NULL);
    assert(transporter != NULL);

    pomelo_allocator_t * allocator = transporter->allocator;
    memset(endpoint, 0, sizeof(pomelo_delivery_endpoint_t));

    endpoint->transporter = transporter;
    endpoint->nbuses = transporter->nbuses;

    // Create array of buses
    size_t buses_size = transporter->nbuses * sizeof(pomelo_delivery_bus_t);
    endpoint->buses = pomelo_allocator_malloc(allocator, buses_size);
    if (!endpoint->buses) {
        // Failed to allocate array of buses
        pomelo_delivery_endpoint_finalize(endpoint, transporter);
        return -1;
    }
    memset(endpoint->buses, 0, buses_size);

    // Initialize all buses
    for (size_t i = 0; i < endpoint->nbuses; ++i) {
        pomelo_delivery_bus_t * bus = endpoint->buses + i;
        int ret = pomelo_delivery_bus_init(bus, endpoint);
        if (ret < 0) {
            // Failed to initialize the bus
            pomelo_delivery_endpoint_finalize(endpoint, transporter);
            return -1;
        }

        bus->index = i; // Update bus index
    }

    return 0;
}


int pomelo_delivery_endpoint_finalize(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_delivery_transporter_t * transporter
) {
    assert(endpoint != NULL);
    (void) transporter;

    if (endpoint->buses) {
        for (size_t i = 0; i < endpoint->nbuses; ++i) {
            pomelo_delivery_bus_finalize(endpoint->buses + i);
        }

        pomelo_allocator_free(
            endpoint->transporter->allocator,
            endpoint->buses
        );

        endpoint->buses = NULL;
    }

    return 0;
}


int pomelo_delivery_endpoint_reset(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_delivery_transporter_t * transporter
) {
    assert(endpoint != NULL);
    (void) transporter;

    for (size_t i = 0; i < endpoint->nbuses; ++i) {
        pomelo_delivery_bus_reset(endpoint->buses + i);
    }

    return 0;
}


pomelo_delivery_bus_t * pomelo_delivery_endpoint_get_bus(
    pomelo_delivery_endpoint_t * endpoint,
    size_t bus_index
) {
    assert(endpoint != NULL);
    if (bus_index >= endpoint->nbuses) {
        return NULL;
    }

    return endpoint->buses + bus_index;
}


int pomelo_delivery_endpoint_recv(
    pomelo_delivery_endpoint_t * endpoint,
    pomelo_buffer_t * buffer,
    size_t offset,
    size_t length
) {
    assert(endpoint != NULL);
    assert(buffer != NULL);

    pomelo_delivery_fragment_meta_t meta = { 0 };
    pomelo_payload_t payload;
    payload.data = buffer->data + offset;
    payload.position = 0;
    payload.capacity = length;

    // Decode the meta. This will limit the payload capacity.
    int ret = pomelo_delivery_fragment_meta_decode(&meta, &payload);
    if (ret < 0) return ret;

    // Check the bus index
    size_t bus_index = meta.bus_index;
    if (bus_index >= endpoint->nbuses) {
        return -1;
    }

    if (meta.total_fragments > endpoint->transporter->context->max_fragments) {
        // More than maximum number of fragments
        return -1;
    }

    // Pass the payload to bus
    pomelo_delivery_bus_t * bus = endpoint->buses + bus_index;
    return pomelo_delivery_bus_on_received_payload(
        bus, &meta, buffer, &payload
    );
}
