#include <assert.h>
#include <string.h>
#include "transporter.h"
#include "commands.h"
#include "endpoint.h"
#include "checksum.h"


/* -------------------------------------------------------------------------- */
/*                             Internal functions                             */
/* -------------------------------------------------------------------------- */


void pomelo_delivery_transporter_options_init(
    pomelo_delivery_transporter_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_delivery_transporter_options_t));
}


pomelo_delivery_transporter_t * pomelo_delivery_transporter_create(
    pomelo_delivery_transporter_options_t * options
) {
    assert(options != NULL);

    if (!options->context) {
        return NULL;
    }

    if (options->nbuses == 0 || options->nbuses > POMELO_DELIVERY_MAX_BUSES) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_delivery_transporter_t * transporter =
        pomelo_allocator_malloc_t(allocator, pomelo_delivery_transporter_t);
    
    if (!transporter) {
        return NULL;
    }

    memset(transporter, 0, sizeof(pomelo_delivery_transporter_t));

    transporter->allocator = allocator;
    transporter->context = options->context;
    transporter->nbuses = options->nbuses;
    transporter->platform = options->platform;

    pomelo_pool_options_t pool_options;

    // Create pool of sending commands
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_send_command_t);
    pool_options.zero_initialized = true;

    transporter->send_pool = pomelo_pool_create(&pool_options);
    if (!transporter->send_pool) {
        pomelo_delivery_transporter_destroy(transporter);
        return NULL;
    }

    // Create pool of receiving comands
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_recv_command_t);
    pool_options.zero_initialized = true;
    
    transporter->recv_pool = pomelo_pool_create(&pool_options);
    if (!transporter->recv_pool) {
        pomelo_delivery_transporter_destroy(transporter);
        return NULL;
    }

    // Create pool of endpoints
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.callback_context = transporter;
    pool_options.element_size = sizeof(pomelo_delivery_endpoint_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_delivery_endpoint_init;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_delivery_endpoint_finalize;
    pool_options.release_callback = (pomelo_pool_callback)
        pomelo_delivery_endpoint_reset;
    
    transporter->endpoint_pool = pomelo_pool_create(&pool_options);
    if (!transporter->endpoint_pool) {
        pomelo_delivery_transporter_destroy(transporter);
        return NULL;
    }

    // Create pool of checksum commands
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_delivery_checksum_command_t);
    pool_options.zero_initialized = true;
    
    transporter->checksum_pool = pomelo_pool_create(&pool_options);
    if (!transporter->checksum_pool) {
        pomelo_delivery_transporter_destroy(transporter);
        return NULL;
    }

    // Create work group
    transporter->task_group =
        pomelo_platform_acquire_task_group(transporter->platform);
    if (!transporter->task_group) {
        pomelo_delivery_transporter_destroy(transporter);
        return NULL;
    }

    // Create list of endpoints (using)
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);

    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_delivery_endpoint_t *);

    transporter->endpoints = pomelo_list_create(&list_options);
    if (!transporter->endpoints) {
        pomelo_delivery_transporter_destroy(transporter);
        return NULL;
    }

    return transporter;
}


void pomelo_delivery_transporter_destroy(
    pomelo_delivery_transporter_t * transporter
) {
    assert(transporter != NULL);

    if (transporter->task_group) {
        pomelo_platform_release_task_group(
            transporter->platform,
            transporter->task_group
        );
        transporter->task_group = NULL;
    }

    if (transporter->endpoint_pool) {
        pomelo_pool_destroy(transporter->endpoint_pool);
        transporter->endpoint_pool = NULL;
    }

    if (transporter->checksum_pool) {
        pomelo_pool_destroy(transporter->checksum_pool);
        transporter->checksum_pool = NULL;
    }

    if (transporter->endpoints) {
        pomelo_list_destroy(transporter->endpoints);
        transporter->endpoints = NULL;
    }

    if (transporter->send_pool) {
        pomelo_pool_destroy(transporter->send_pool);
        transporter->send_pool = NULL;
    }

    if (transporter->recv_pool) {
        pomelo_pool_destroy(transporter->recv_pool);
        transporter->recv_pool = NULL;
    }

    pomelo_allocator_free(transporter->allocator, transporter);
}


pomelo_delivery_endpoint_t * pomelo_delivery_transporter_acquire_endpoint(
    pomelo_delivery_transporter_t * transporter
) {
    assert(transporter != NULL);
    pomelo_delivery_endpoint_t * endpoint =
        pomelo_pool_acquire(transporter->endpoint_pool);
    if (!endpoint) {
        return NULL;
    }

    endpoint->list_node =
        pomelo_list_push_back(transporter->endpoints, endpoint);

    if (!endpoint->list_node) {
        pomelo_pool_release(transporter->endpoint_pool, endpoint);
        return NULL;
    }
    
    return endpoint;
}


void pomelo_delivery_transporter_release_endpoint(
    pomelo_delivery_transporter_t * transporter,
    pomelo_delivery_endpoint_t * endpoint
) {
    assert(transporter != NULL);
    assert(endpoint != NULL);

    pomelo_pool_release(transporter->endpoint_pool, endpoint);
    pomelo_list_remove(transporter->endpoints, endpoint->list_node);
}


void pomelo_delivery_transporter_stop(
    pomelo_delivery_transporter_t * transporter
) {
    assert(transporter != NULL);

    // Stop all works
    pomelo_platform_cancel_task_group(
        transporter->platform,
        transporter->task_group,
        (pomelo_platform_task_cb) pomelo_delivery_transporter_stop_deferred,
        transporter
    );
}


void pomelo_delivery_transporter_statistic(
    pomelo_delivery_transporter_t * transporter,
    pomelo_statistic_delivery_t * statistic
) {
    assert(transporter != NULL);
    assert(statistic != NULL);

    statistic->endpoints = pomelo_pool_in_use(transporter->endpoint_pool);
    statistic->send_commands = pomelo_pool_in_use(transporter->send_pool);
    statistic->recv_commands = pomelo_pool_in_use(transporter->recv_pool);

    pomelo_delivery_context_t * context = transporter->context;
    context->statistic(context, statistic);
}


void pomelo_delivery_transporter_stop_deferred(
    pomelo_delivery_transporter_t * transporter
) {

    pomelo_list_t * endpoints = transporter->endpoints;
    pomelo_pool_t * pool = transporter->endpoint_pool;

    // By releasing all endpoints, the schedules will be stopped as well.
    pomelo_delivery_endpoint_t * endpoint;
    pomelo_list_ptr_for(endpoints, endpoint, {
        pomelo_pool_release(pool, endpoint);
    });

    pomelo_list_clear(endpoints);
    pomelo_delivery_transporter_on_stopped(transporter);
}
