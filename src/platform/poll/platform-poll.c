#include <assert.h>
#include <string.h>
#include "platform-poll.h"


void pomelo_platform_poll_options_init(
    pomelo_platform_poll_options_t * options
) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_platform_poll_options_t));
}


pomelo_platform_t * pomelo_platform_poll_create(
    pomelo_platform_poll_options_t * options
) {
    assert(options != NULL);
    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_platform_t * platform =
        pomelo_allocator_malloc_t(allocator, pomelo_platform_t);
    if (!platform) {
        return NULL;
    }
    memset(platform, 0, sizeof(pomelo_platform_t));

    platform->allocator = allocator;
    pomelo_extra_set(platform->extra, NULL);

    uv_loop_t * uv_loop = &platform->uv_loop;
    uv_loop_init(uv_loop);
    
    platform->udp_controller =
        pomelo_platform_udp_controller_create(allocator, uv_loop);
    if (!platform->udp_controller) {
        pomelo_platform_poll_destroy(platform);
        return NULL;
    }
    
    // Create timer controller
    platform->timer_controller =
        pomelo_platform_timer_controller_create(allocator, uv_loop);
    if (!platform->timer_controller) {
        pomelo_platform_poll_destroy(platform);
        return NULL;
    }

    // Create task controller
    platform->task_controller =
        pomelo_platform_task_controller_create(allocator, uv_loop);
    if (!platform->task_controller) {
        pomelo_platform_poll_destroy(platform);
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
    pool_options.release_callback = (pomelo_pool_callback)
        pomelo_platform_task_group_finalize;
    platform->task_group_pool = pomelo_pool_create(&pool_options);
    if (!platform->task_group_pool) {
        pomelo_platform_poll_destroy(platform);
        return NULL;
    }

    return platform;
}


void pomelo_platform_poll_destroy(pomelo_platform_t * platform) {
    assert(platform != NULL);

    uv_loop_close(&platform->uv_loop);

    if (platform->timer_controller) {
        pomelo_platform_timer_controller_destroy(platform->timer_controller);
        platform->timer_controller = NULL;
    }

    if (platform->udp_controller) {
        pomelo_platform_udp_controller_destroy(platform->udp_controller);
        platform->udp_controller = NULL;
    }

    if (platform->task_controller) {
        pomelo_platform_task_controller_destroy(platform->task_controller);
        platform->task_controller = NULL;
    }

    if (platform->task_group_pool) {
        pomelo_pool_destroy(platform->task_group_pool);
        platform->task_group_pool = NULL;
    }

    pomelo_allocator_free(platform->allocator, platform);    
}


int pomelo_platform_poll_service(pomelo_platform_t * platform) {
    assert(platform != NULL);
    int uv_ret = uv_run(&platform->uv_loop, UV_RUN_NOWAIT);
    int task_ret =
        pomelo_platform_task_controller_service(platform->task_controller);
    if (uv_ret < 0 || task_ret < 0) {
        return -1;
    }
    return (uv_ret || task_ret);
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

    pomelo_platform_task_controller_statistic(
        platform->task_controller,
        statistic
    );

    statistic->task_groups = pomelo_pool_in_use(platform->task_group_pool);
}


void pomelo_platform_startup(pomelo_platform_t * platform) {
    assert(platform != NULL);

    pomelo_platform_udp_controller_startup(platform->udp_controller);
    pomelo_platform_timer_controller_startup(platform->timer_controller);
    pomelo_platform_task_controller_startup(platform->task_controller);
}


void pomelo_platform_shutdown(pomelo_platform_t * platform) {
    assert(platform != NULL);

    pomelo_platform_udp_controller_shutdown(platform->udp_controller);
    pomelo_platform_timer_controller_shutdown(platform->timer_controller);
    pomelo_platform_task_controller_shutdown(platform->task_controller);
}


/* -------------------------------------------------------------------------- */
/*                            Platform Task APIs                              */
/* -------------------------------------------------------------------------- */

/// @brief Create new task group
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
    return pomelo_platform_task_controller_submit_late(
        platform->task_controller,
        group,
        callback,
        /* done = */ NULL,
        callback_data
    );
}


int pomelo_platform_submit_main_task(
    pomelo_platform_t * platform,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(platform != NULL);
    return pomelo_platform_task_controller_submit_main(
        platform->task_controller,
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
    return pomelo_platform_task_controller_submit_late(
        platform->task_controller,
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
