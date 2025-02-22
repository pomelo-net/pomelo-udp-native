#include <assert.h>
#include <string.h>
#include "uv.h"
#include "utils/list.h"
#include "platform-uv.h"
#include "platform-task-group.h"


void pomelo_platform_set_extra(pomelo_platform_t * platform, void * data) {
    assert(platform != NULL);
    pomelo_extra_set(platform->extra, data);
}


void * pomelo_platform_get_extra(pomelo_platform_t * platform) {
    assert(platform != NULL);
    return pomelo_extra_get(platform->extra);
}


void pomelo_platform_uv_options_init(pomelo_platform_uv_options_t * options) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_platform_uv_options_t));
}


pomelo_platform_t * pomelo_platform_uv_create(
    pomelo_platform_uv_options_t * options
) {
    assert(options != NULL);
    if (!options->uv_loop) {
        return NULL;
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_platform_t * platform = 
        pomelo_allocator_malloc_t(allocator, pomelo_platform_t);
    if (!platform) {
        // Cannot allocate
        return NULL;
    }
    memset(platform, 0, sizeof(pomelo_platform_t));
    pomelo_extra_set(platform->extra, NULL);

    uv_loop_t * uv_loop = options->uv_loop;

    platform->allocator = allocator;
    platform->uv_loop = uv_loop;

    platform->task_deferred_controller =
        pomelo_platform_task_deferred_controller_create(allocator, uv_loop);
    if (!platform->task_deferred_controller) {
        pomelo_platform_uv_destroy(platform);
        return NULL;
    }

    platform->task_worker_controller = 
        pomelo_platform_task_worker_controller_create(allocator, uv_loop);
    if (!platform->task_worker_controller) {
        pomelo_platform_uv_destroy(platform);
        return NULL;
    }

    platform->task_main_controller =
        pomelo_platform_task_main_controller_create(allocator, uv_loop);
    if (!platform->task_main_controller) {
        pomelo_platform_uv_destroy(platform);
        return NULL;
    }

    platform->udp_controller =
        pomelo_platform_udp_controller_create(allocator, uv_loop);
    if (!platform->udp_controller) {
        pomelo_platform_uv_destroy(platform);
        return NULL;
    }
    
    // Create timer manager
    platform->timer_controller =
        pomelo_platform_timer_controller_create(allocator, uv_loop);
    if (!platform->timer_controller) {
        pomelo_platform_uv_destroy(platform);
        return NULL;
    }

    // Create task group pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.callback_context = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_group_t);
    pool_options.allocate_callback = (pomelo_pool_callback)
        pomelo_platform_task_group_init;
    pool_options.acquire_callback = (pomelo_pool_callback)
        pomelo_platform_task_group_reset;
    pool_options.deallocate_callback = (pomelo_pool_callback)
        pomelo_platform_task_group_finalize;
    platform->task_group_pool = pomelo_pool_create(&pool_options);
    if (!platform->task_group_pool) {
        pomelo_platform_uv_destroy(platform);
        return NULL;
    }

    return platform;
}


void pomelo_platform_uv_destroy(pomelo_platform_t * platform) {
    assert(platform != NULL);

    if (platform->timer_controller) {
        pomelo_platform_timer_controller_destroy(platform->timer_controller);
        platform->timer_controller = NULL;
    }

    if (platform->task_deferred_controller) {
        pomelo_platform_task_deferred_controller_destroy(
            platform->task_deferred_controller
        );
        platform->task_deferred_controller = NULL;
    }

    if (platform->task_main_controller) {
        pomelo_platform_task_main_controller_destroy(
            platform->task_main_controller
        );
        platform->task_main_controller = NULL;
    }

    if (platform->task_worker_controller) {
        pomelo_platform_task_worker_controller_destroy(
            platform->task_worker_controller
        );
        platform->task_worker_controller = NULL;
    }

    if (platform->udp_controller) {
        pomelo_platform_udp_controller_destroy(platform->udp_controller);
        platform->udp_controller = NULL;
    }

    if (platform->task_group_pool) {
        pomelo_pool_destroy(platform->task_group_pool);
        platform->task_group_pool = NULL;
    }

    pomelo_allocator_free(platform->allocator, platform);    
}


void pomelo_platform_statistic(
    pomelo_platform_t * platform,
    pomelo_statistic_platform_t * statistic
) {
    assert(platform != NULL);
    assert(statistic != NULL);

    pomelo_platform_timer_controller_statistic(
        platform->timer_controller,
        statistic
    );

    pomelo_platform_udp_controller_statistic(
        platform->udp_controller,
        statistic
    );

    pomelo_platform_task_deferred_controller_statistic(
        platform->task_deferred_controller,
        statistic
    );

    pomelo_platform_task_main_controller_statistic(
        platform->task_main_controller,
        statistic
    );

    pomelo_platform_task_worker_controller_statistic(
        platform->task_worker_controller,
        statistic
    );
}


uv_loop_t * pomelo_platform_uv_get_uv_loop(pomelo_platform_t * platform) {
    assert(platform != NULL);
    return platform->uv_loop;
}


void pomelo_platform_startup(pomelo_platform_t * platform) {
    assert(platform != NULL);

    pomelo_platform_udp_controller_startup(platform->udp_controller);
    pomelo_platform_timer_controller_startup(platform->timer_controller);
    pomelo_platform_task_deferred_controller_startup(
        platform->task_deferred_controller
    );
    pomelo_platform_task_main_controller_startup(
        platform->task_main_controller
    );
    pomelo_platform_task_worker_controller_startup(
        platform->task_worker_controller
    );
}


void pomelo_platform_shutdown(pomelo_platform_t * platform) {
    assert(platform != NULL);

    pomelo_platform_udp_controller_shutdown(platform->udp_controller);
    pomelo_platform_timer_controller_shutdown(platform->timer_controller);
    pomelo_platform_task_deferred_controller_shutdown(
        platform->task_deferred_controller
    );
    pomelo_platform_task_main_controller_shutdown(
        platform->task_main_controller
    );
    pomelo_platform_task_worker_controller_shutdown(
        platform->task_worker_controller
    );
}


/* -------------------------------------------------------------------------- */
/*                            Platform Task APIs                              */
/* -------------------------------------------------------------------------- */

pomelo_platform_task_group_t * pomelo_platform_acquire_task_group(
    pomelo_platform_t * platform
) {
    assert(platform != NULL);
    return pomelo_pool_acquire(platform->task_group_pool);
}


void pomelo_platform_release_task_group(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group
) {
    assert(platform != NULL);
    pomelo_pool_release(platform->task_group_pool, group);
}


int pomelo_platform_submit_deferred_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(platform != NULL);
    return pomelo_platform_task_deferred_controller_submit(
        platform->task_deferred_controller,
        group,
        callback,
        callback_data
    );
}


int pomelo_platform_submit_main_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(platform != NULL);
    return pomelo_platform_task_main_controller_submit(
        platform->task_main_controller,
        callback,
        callback_data
    );
}


int pomelo_platform_submit_worker_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb entry,
    pomelo_platform_task_done_cb done,
    void * callback_data
) {
    assert(platform != NULL);
    return pomelo_platform_task_worker_controller_submit(
        platform->task_worker_controller,
        group,
        entry,
        done,
        callback_data
    );
}


/* -------------------------------------------------------------------------- */
/*                             Platform UDP APIs                              */
/* -------------------------------------------------------------------------- */

pomelo_platform_udp_t * pomelo_platform_udp_bind(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    return pomelo_platform_udp_controller_bind(
        platform->udp_controller,
        address
    );
}


pomelo_platform_udp_t * pomelo_platform_udp_connect(
    pomelo_platform_t * platform,
    pomelo_address_t * address
) {
    assert(platform != NULL);
    return pomelo_platform_udp_controller_connect(
        platform->udp_controller,
        address
    );
}


int pomelo_platform_udp_stop(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket
) {
    assert(platform != NULL);
    return pomelo_platform_udp_controller_stop(
        platform->udp_controller,
        socket
    );
}


int pomelo_platform_udp_send(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    pomelo_address_t * address,
    int nbuffers,
    pomelo_buffer_vector_t * buffers,
    void * callback_data
) {
    assert(platform != NULL);
    return pomelo_platform_udp_controller_send(
        platform->udp_controller,
        socket,
        address,
        nbuffers,
        buffers,
        callback_data
    );
}


void pomelo_platform_udp_set_callbacks(
    pomelo_platform_t * platform,
    pomelo_platform_udp_t * socket,
    void * context,
    pomelo_platform_alloc_cb alloc_callback,
    pomelo_platform_recv_cb recv_callback,
    pomelo_platform_send_cb send_callback
) {
    assert(platform != NULL);
    pomelo_platform_udp_controller_set_callbacks(
        platform->udp_controller,
        socket,
        context,
        alloc_callback,
        recv_callback,
        send_callback
    );
}


/* -------------------------------------------------------------------------- */
/*                            Platform Timer APIs                             */
/* -------------------------------------------------------------------------- */


pomelo_platform_timer_t * pomelo_platform_timer_start(
    pomelo_platform_t * platform,
    pomelo_platform_timer_cb callback,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * callback_data
) {
    assert(platform != NULL);
    return pomelo_platform_timer_controller_start(
        platform->timer_controller,
        callback,
        timeout_ms,
        repeat_ms,
        callback_data
    );
}


int pomelo_platform_timer_stop(
    pomelo_platform_t * platform,
    pomelo_platform_timer_t * timer
) {
    assert(platform != NULL);
    return pomelo_platform_timer_controller_stop(
        platform->timer_controller,
        timer
    );
}
