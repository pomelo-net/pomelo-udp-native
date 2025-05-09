#include <assert.h>
#include "utils/macro.h"
#include "heartbeat.h"
#include "context.h"
#include "endpoint.h"


pomelo_delivery_heartbeat_t * pomelo_delivery_heartbeat_create(
    pomelo_delivery_heartbeat_options_t * options
) {
    assert(options != NULL);
    if (!options->context || !options->platform) return NULL; // Invalid options
    return pomelo_pool_acquire(options->context->heartbeat_pool, options);
}


void pomelo_delivery_heartbeat_destroy(
    pomelo_delivery_heartbeat_t * heartbeat
) {
    assert(heartbeat != NULL);
    pomelo_pool_release(heartbeat->context->heartbeat_pool, heartbeat);
}


int pomelo_delivery_heartbeat_on_alloc(
    pomelo_delivery_heartbeat_t * heartbeat,
    pomelo_delivery_context_t * context
) {
    assert(heartbeat != NULL);
    heartbeat->context = context;

    // Create list of endpoints
    pomelo_list_options_t options = {
        .allocator = context->allocator,
        .element_size = sizeof(pomelo_delivery_endpoint_t *)
    };
    heartbeat->endpoints = pomelo_list_create(&options);
    if (!heartbeat->endpoints) return -1; // Failed to create list of endpoints

    return 0;
}


void pomelo_delivery_heartbeat_on_free(
    pomelo_delivery_heartbeat_t * heartbeat
) {
    assert(heartbeat != NULL);
    heartbeat->context = NULL;

    if (heartbeat->endpoints) {
        pomelo_list_destroy(heartbeat->endpoints);
        heartbeat->endpoints = NULL;
    }
}


int pomelo_delivery_heartbeat_init(
    pomelo_delivery_heartbeat_t * heartbeat,
    pomelo_delivery_heartbeat_options_t * options
) {
    assert(heartbeat != NULL);
    assert(options != NULL);
    heartbeat->platform = options->platform;
    return 0;
}


/// @brief Cleanup heartbeat
void pomelo_delivery_heartbeat_cleanup(
    pomelo_delivery_heartbeat_t * heartbeat
) {
    assert(heartbeat != NULL);
    // Stop the timer
    pomelo_platform_timer_stop(heartbeat->platform, &heartbeat->timer_handle);

    // Remove all endpoints and their handles
    pomelo_delivery_endpoint_t * endpoint = NULL;
    while (pomelo_list_pop_front(heartbeat->endpoints, &endpoint) == 0) {
        endpoint->heartbeat_handle.entry = NULL;
    }
}


int pomelo_delivery_heartbeat_schedule(
    pomelo_delivery_heartbeat_t * heartbeat,
    pomelo_delivery_endpoint_t * endpoint
) {
    assert(heartbeat != NULL);
    assert(endpoint != NULL);

    // Get the handle
    pomelo_delivery_heartbeat_handle_t * handle = &endpoint->heartbeat_handle;
    if (handle->entry) return -1; // Already scheduled

    pomelo_list_t * endpoints = heartbeat->endpoints;
    if (endpoints->size == 0) {
        // Schedule the timer
        int ret = pomelo_platform_timer_start(
            heartbeat->platform,
            (pomelo_platform_timer_entry) pomelo_delivery_heartbeat_run,
            POMELO_FREQ_TO_MS(POMELO_DELIVERY_HEARTBEAT_FREQUENCY),
            POMELO_FREQ_TO_MS(POMELO_DELIVERY_HEARTBEAT_FREQUENCY),
            heartbeat,
            &heartbeat->timer_handle
        );
        if (ret < 0) return ret; // Failed to start timer
    }

    // Append endpoint to the list
    handle->entry = pomelo_list_push_back(heartbeat->endpoints, endpoint);
    if (!handle->entry) {
        // Failed to append
        if (endpoints->size == 0) {
            // Stop the timer
            pomelo_platform_timer_stop(
                heartbeat->platform,
                &heartbeat->timer_handle
            );
        }

        return -1;
    }

    return 0;
}


void pomelo_delivery_heartbeat_unschedule(
    pomelo_delivery_heartbeat_t * heartbeat,
    pomelo_delivery_endpoint_t * endpoint
) {
    assert(heartbeat != NULL);
    assert(endpoint != NULL);

    pomelo_delivery_heartbeat_handle_t * handle = &endpoint->heartbeat_handle;
    if (!handle->entry) return; // Not scheduled before
    pomelo_list_t * endpoints = heartbeat->endpoints;

    // Remove the endpoint from list
    pomelo_list_remove(endpoints, handle->entry);
    handle->entry = NULL;

    // Stop the timer if there's no more endpoints
    if (endpoints->size == 0) {
        pomelo_platform_timer_stop(
            heartbeat->platform,
            &heartbeat->timer_handle
        );
    }
}


void pomelo_delivery_heartbeat_run(pomelo_delivery_heartbeat_t * heartbeat) {
    assert(heartbeat != NULL);

    pomelo_list_iterator_t it;
    pomelo_delivery_endpoint_t * endpoint;
    pomelo_list_iterator_init(&it, heartbeat->endpoints);
    while (pomelo_list_iterator_next(&it, &endpoint) == 0) {
        pomelo_delivery_endpoint_heartbeat(endpoint);
    }
}
