#include <assert.h>
#include <string.h>
#include "uv.h"
#include "utils/list.h"
#include "platform-uv.h"
#include "executor.h"
#include "worker.h"
#include "timer.h"
#include "udp.h"


void pomelo_platform_uv_set_extra(
    pomelo_platform_uv_t * platform,
    void * data
) {
    assert(platform != NULL);
    pomelo_extra_set(platform->extra, data);
}


void * pomelo_platform_uv_get_extra(pomelo_platform_uv_t * platform) {
    assert(platform != NULL);
    return pomelo_extra_get(platform->extra);
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

    pomelo_platform_uv_t * platform = 
        pomelo_allocator_malloc_t(allocator, pomelo_platform_uv_t);
    if (!platform) return NULL; // Failed to allocate new platform

    memset(platform, 0, sizeof(pomelo_platform_uv_t));
    pomelo_extra_set(platform->extra, NULL);

    uv_loop_t * uv_loop = options->uv_loop;
    platform->allocator = allocator;
    platform->uv_loop = uv_loop;

    // Initialize idle handle for shutdown
    uv_idle_init(uv_loop, &platform->shutdown_idle);
    platform->shutdown_idle.data = platform;

    platform->worker_controller = 
        pomelo_platform_worker_controller_create(platform, allocator, uv_loop);
    if (!platform->worker_controller) {
        pomelo_platform_uv_destroy((pomelo_platform_t *) platform);
        return NULL;
    }

    platform->threadsafe_controller =
        pomelo_platform_threadsafe_controller_create(
            platform, allocator, uv_loop
        );
    if (!platform->threadsafe_controller) {
        pomelo_platform_uv_destroy((pomelo_platform_t *) platform);
        return NULL;
    }

    platform->udp_controller = pomelo_platform_udp_controller_create(
        platform, allocator, uv_loop
    );
    if (!platform->udp_controller) {
        pomelo_platform_uv_destroy((pomelo_platform_t *) platform);
        return NULL;
    }
    
    // Create timer manager
    platform->timer_controller = pomelo_platform_timer_controller_create(
        platform, allocator, uv_loop
    );
    if (!platform->timer_controller) {
        pomelo_platform_uv_destroy((pomelo_platform_t *) platform);
        return NULL;
    }

    return (pomelo_platform_t *) platform;
}


void pomelo_platform_uv_destroy(pomelo_platform_t * platform) {
    assert(platform != NULL);
    pomelo_platform_uv_t * uv_platform = (pomelo_platform_uv_t *) platform;

    if (uv_platform->timer_controller) {
        pomelo_platform_timer_controller_destroy(uv_platform->timer_controller);
        uv_platform->timer_controller = NULL;
    }

    if (uv_platform->threadsafe_controller) {
        pomelo_platform_threadsafe_controller_destroy(
            uv_platform->threadsafe_controller
        );
        uv_platform->threadsafe_controller = NULL;
    }

    if (uv_platform->worker_controller) {
        pomelo_platform_worker_controller_destroy(
            uv_platform->worker_controller
        );
        uv_platform->worker_controller = NULL;
    }

    if (uv_platform->udp_controller) {
        pomelo_platform_udp_controller_destroy(uv_platform->udp_controller);
        uv_platform->udp_controller = NULL;
    }

    pomelo_allocator_free(uv_platform->allocator, uv_platform);    
}


void pomelo_platform_uv_statistic(
    pomelo_platform_t * platform,
    pomelo_statistic_platform_uv_t * statistic
) {
    assert(platform != NULL);
    assert(statistic != NULL);

    pomelo_platform_uv_t * uv_platform = (pomelo_platform_uv_t *) platform;

    pomelo_platform_timer_controller_statistic(
        uv_platform->timer_controller,
        statistic
    );

    pomelo_platform_udp_controller_statistic(
        uv_platform->udp_controller,
        statistic
    );

    pomelo_platform_threadsafe_controller_statistic(
        uv_platform->threadsafe_controller,
        statistic
    );

    pomelo_platform_worker_controller_statistic(
        uv_platform->worker_controller,
        statistic
    );
}


uv_loop_t * pomelo_platform_uv_get_uv_loop(pomelo_platform_t * platform) {
    assert(platform != NULL);
    return ((pomelo_platform_uv_t *) platform)->uv_loop;
}


void pomelo_platform_uv_startup(pomelo_platform_uv_t * platform) {
    assert(platform != NULL);

    platform->running = true;
    pomelo_platform_udp_controller_startup(platform->udp_controller);
    pomelo_platform_timer_controller_startup(platform->timer_controller);
    pomelo_platform_threadsafe_controller_startup(
        platform->threadsafe_controller
    );
    pomelo_platform_worker_controller_startup(platform->worker_controller);
}


static void shutdown_idle_cb(uv_idle_t * handle) {
    pomelo_platform_uv_t * platform = handle->data;
    assert(platform != NULL);

    uv_idle_stop(handle);

    pomelo_platform_udp_controller_shutdown(platform->udp_controller);
    pomelo_platform_timer_controller_shutdown(platform->timer_controller);
    pomelo_platform_threadsafe_controller_shutdown(
        platform->threadsafe_controller
    );
    pomelo_platform_worker_controller_shutdown(platform->worker_controller);
}


void pomelo_platform_uv_shutdown(
    pomelo_platform_uv_t * platform,
    pomelo_platform_shutdown_callback callback
) {
    assert(platform != NULL);
    if (!platform->running) return; // Already shutting down

    platform->running = false;
    platform->shutdown_callback = callback;
    platform->shutdown_components = 0;

    uv_idle_start(&platform->shutdown_idle, shutdown_idle_cb);
}


void pomelo_platform_threadsafe_controller_on_shutdown(
    pomelo_platform_threadsafe_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_platform_uv_t * platform = controller->platform;
    assert(platform != NULL);
    assert(!platform->running);

    platform->shutdown_components |= POMELO_PLATFORM_UV_COMPONENT_THREADSAFE;
    pomelo_platform_check_shutdown(platform);
}


void pomelo_platform_worker_controller_on_shutdown(
    pomelo_platform_worker_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_platform_uv_t * platform = controller->platform;
    assert(platform != NULL);
    assert(!platform->running);

    platform->shutdown_components |= POMELO_PLATFORM_UV_COMPONENT_WORKER;
    pomelo_platform_check_shutdown(platform);
}


void pomelo_platform_udp_controller_on_shutdown(
    pomelo_platform_udp_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_platform_uv_t * platform = controller->platform;
    assert(platform != NULL);
    assert(!platform->running);

    platform->shutdown_components |= POMELO_PLATFORM_UV_COMPONENT_UDP;
    pomelo_platform_check_shutdown(platform);
}


void pomelo_platform_timer_controller_on_shutdown(
    pomelo_platform_timer_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_platform_uv_t * platform = controller->platform;
    assert(platform != NULL);
    assert(!platform->running);

    platform->shutdown_components |= POMELO_PLATFORM_UV_COMPONENT_TIMER;
    pomelo_platform_check_shutdown(platform);
}


void pomelo_platform_check_shutdown(pomelo_platform_uv_t * platform) {
    assert(platform != NULL);
    assert(!platform->running);

    if (platform->shutdown_components == POMELO_PLATFORM_UV_COMPONENT_ALL &&
        platform->shutdown_callback
    ) {
        platform->shutdown_callback((pomelo_platform_t *) platform);
    }
}
