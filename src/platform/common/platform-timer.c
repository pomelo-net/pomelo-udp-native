#include <assert.h>
#include <stdbool.h>
#include "platform-timer.h"


pomelo_platform_timer_controller_t * pomelo_platform_timer_controller_create(
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

    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    // Create active timers list
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_timer_t *);

    controller->timers = pomelo_list_create(&list_options);
    if (!controller->timers) {
        pomelo_platform_timer_controller_destroy(controller);
        return NULL;
    }

    // Create timer pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_timer_t);

    controller->timer_pool = pomelo_pool_create(&pool_options);

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

    // Stop all timersz
    pomelo_platform_timer_t * timer;
    pomelo_list_ptr_for(controller->timers, timer, {
        uv_close((uv_handle_t *) &timer->uv_timer, NULL);
        timer->is_running = 0;
    });

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
    pomelo_statistic_platform_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);

    statistic->timers = controller->timers->size;
}


void pomelo_platform_timer_controller_startup(
    pomelo_platform_timer_controller_t * controller
) {
    (void) controller;
    // Ignore
}


void pomelo_platform_timer_controller_shutdown(
    pomelo_platform_timer_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_platform_timer_t * timer;
    pomelo_list_ptr_for(controller->timers, timer, {
        pomelo_platform_timer_stop_ex(controller, timer);
    });
}

pomelo_platform_timer_t * pomelo_platform_timer_controller_start(
    pomelo_platform_timer_controller_t * controller,
    pomelo_platform_timer_cb callback,
    uint64_t timeout_ms,
    uint64_t repeat_ms,
    void * callback_data
) {
    assert(controller != NULL);
    assert(callback != NULL);

    pomelo_platform_timer_t * timer =
        pomelo_pool_acquire(controller->timer_pool);
    if (!timer) {
        // Cannot allocate new timer
        return NULL;
    }

    timer->controller = controller;
    timer->data = callback_data;
    timer->callback = callback;
    timer->is_repeat = (repeat_ms != 0);
    timer->is_running = true;

    timer->list_node = pomelo_list_push_back(controller->timers, timer);
    if (!timer->list_node) {
        // Cannot add new timer to active list
        pomelo_pool_release(controller->timer_pool, timer);
        return NULL;
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
        pomelo_list_remove(controller->timers, timer->list_node);
        pomelo_pool_release(controller->timer_pool, timer);
        return NULL;
    }

    return timer;
}



int pomelo_platform_timer_controller_stop(
    pomelo_platform_timer_controller_t * controller,
    pomelo_platform_timer_t * timer
) {
    assert(controller != NULL);
    assert(timer != NULL);

    return pomelo_platform_timer_stop_ex(controller, timer);
}



/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


int pomelo_platform_timer_stop_ex(
    pomelo_platform_timer_controller_t * controller,
    pomelo_platform_timer_t * timer
) {
    assert(controller != NULL);
    assert(timer != NULL);

    if (!timer->is_running) {
        // The timer is not running. Nothing to do
        return 0;
    }

    uv_timer_t * uv_timer = &timer->uv_timer;

    // Stop the UV timer
    uv_timer_stop(uv_timer);
    timer->is_running = false;

    // Remove timer from active list and release it
    pomelo_list_remove(controller->timers, timer->list_node);
    pomelo_pool_release(controller->timer_pool, timer);

    return 0;
}


void pomelo_platform_uv_timer_callback(uv_timer_t * uv_timer) {
    assert(uv_timer->data != NULL);

    pomelo_platform_timer_t * timer = uv_timer->data;
    pomelo_platform_timer_cb callback = timer->callback;
    void * callback_data = timer->data;

    if (!timer->is_repeat) {
        // Timer is not repeating, stop running
        uv_close((uv_handle_t *) uv_timer, NULL);
        timer->is_running = 0;

        // Release the timer
        pomelo_pool_release(timer->controller->timer_pool, timer);

        // Remove timer from list
        pomelo_list_remove(timer->controller->timers, timer->list_node);
    }

    callback(callback_data);
}
