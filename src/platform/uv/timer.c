#include <assert.h>
#include <stdbool.h>
#include "timer.h"


pomelo_platform_timer_controller_t * pomelo_platform_timer_controller_create(
    pomelo_platform_uv_t * platform,
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);
    assert(uv_loop != NULL);

    pomelo_platform_timer_controller_t * controller = pomelo_allocator_malloc(
        allocator,
        sizeof(pomelo_platform_timer_controller_t)
    );

    if (!controller) {
        return NULL;
    }

    memset(controller, 0, sizeof(pomelo_platform_timer_controller_t));
    controller->platform = platform;
    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    // Create active timers list
    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_platform_timer_t *)
    };
    controller->timers = pomelo_list_create(&list_options);
    if (!controller->timers) {
        pomelo_platform_timer_controller_destroy(controller);
        return NULL;
    }

    // Create timer pool
    pomelo_pool_root_options_t pool_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_platform_timer_t),
        .zero_init = true
    };
    controller->timer_pool = pomelo_pool_root_create(&pool_options);

    if (!controller->timer_pool) {
        pomelo_platform_timer_controller_destroy(controller);
        return NULL;
    }

    return controller;
}



void pomelo_platform_timer_controller_destroy(
    pomelo_platform_timer_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_allocator_t * allocator = controller->allocator;

    if (controller->timers) {
        pomelo_list_destroy(controller->timers);
        controller->timers = NULL;
    }

    if (controller->timer_pool) {
        pomelo_pool_destroy(controller->timer_pool);
        controller->timer_pool = NULL;
    }

    pomelo_allocator_free(allocator, controller);
}


void pomelo_platform_timer_controller_statistic(
    pomelo_platform_timer_controller_t * controller,
    pomelo_statistic_platform_uv_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);

    statistic->timers = controller->timers->size;
}


void pomelo_platform_timer_controller_startup(
    pomelo_platform_timer_controller_t * controller
) {
    assert(controller != NULL);
    controller->running = true;
}


void pomelo_platform_timer_controller_shutdown(
    pomelo_platform_timer_controller_t * controller
) {
    assert(controller != NULL);
    if (!controller->running) {
        return; // Controller is already shutting down
    }
    controller->running = false;
    if (controller->timers->size == 0) {
        pomelo_platform_timer_controller_on_shutdown(controller);
        return;
    }

    pomelo_platform_timer_t * timer = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, controller->timers);
    while (pomelo_list_iterator_next(&it, &timer) == 0) {
        pomelo_platform_uv_timer_stop_ex(timer);
    }
}


int pomelo_platform_uv_timer_start(
    pomelo_platform_uv_t * platform,
    pomelo_platform_timer_entry entry,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * data,
    pomelo_platform_timer_handle_t * handle
) {
    assert(platform != NULL);
    assert(entry != NULL);

    pomelo_platform_timer_controller_t * controller =
        platform->timer_controller;

    pomelo_platform_timer_t * timer =
        pomelo_pool_acquire(controller->timer_pool, NULL);
    if (!timer) return -1; // Cannot allocate new timer

    timer->controller = controller;
    timer->data = data;
    timer->entry = entry;
    timer->is_repeat = (repeat_ms != 0);
    timer->is_running = true;

    timer->list_entry = pomelo_list_push_back(controller->timers, timer);
    if (!timer->list_entry) {
        // Cannot add new timer to active list
        pomelo_pool_release(controller->timer_pool, timer);
        return -1;
    }

    uv_timer_init(controller->uv_loop, &timer->uv_timer);
    timer->uv_timer.data = timer;

    int ret = uv_timer_start(
        &timer->uv_timer,
        pomelo_platform_uv_timer_callback,
        timeout_ms,
        repeat_ms
    );

    if (ret < 0) {
        // Cannot start the timer
        pomelo_list_remove(controller->timers, timer->list_entry);
        pomelo_pool_release(controller->timer_pool, timer);
        return -1;
    }

    if (handle) {
        handle->timer = timer;
        timer->handle = handle;
    }

    return 0;
}


void pomelo_platform_uv_timer_stop(
    pomelo_platform_uv_t * platform,
    pomelo_platform_timer_handle_t * handle
) {
    (void) platform;
    assert(handle != NULL);
    if (!handle->timer) return; // No timer

    pomelo_platform_uv_timer_stop_ex(handle->timer);
    handle->timer = NULL;
}



/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

/// @brief Callback for when the timer handle is closed
static void uv_timer_stop_complete(uv_handle_t * handle) {
    pomelo_platform_timer_t * timer = handle->data;

    // Remove timer from active list and release it
    pomelo_platform_timer_controller_t * controller = timer->controller;
    pomelo_list_remove(controller->timers, timer->list_entry);
    pomelo_pool_release(controller->timer_pool, timer);

    if (!controller->running && controller->timers->size == 0) {
        pomelo_platform_timer_controller_on_shutdown(controller);
    }
}


int pomelo_platform_uv_timer_stop_ex(pomelo_platform_timer_t * timer) {
    assert(timer != NULL);
    if (!timer->is_running) {
        return 0; // The timer is not running. Nothing to do
    }

    // Stop the UV timer
    timer->is_running = false;
    uv_close((uv_handle_t *) &timer->uv_timer, uv_timer_stop_complete);

    if (timer->handle) {
        timer->handle->timer = NULL;
        timer->handle = NULL;
    }

    return 0;
}


void pomelo_platform_uv_timer_callback(uv_timer_t * uv_timer) {
    assert(uv_timer->data != NULL);

    pomelo_platform_timer_t * timer = uv_timer->data;
    pomelo_platform_timer_entry entry = timer->entry;
    void * data = timer->data;

    if (!timer->is_repeat) {
        // Timer is not repeating, stop running
        pomelo_platform_uv_timer_stop_ex(timer);
    }

    entry(data);
}
