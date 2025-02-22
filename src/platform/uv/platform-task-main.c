#include <assert.h>
#include <string.h>
#include "platform-task-main.h"
#include "platform-task-group.h"


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


pomelo_platform_task_main_controller_t *
pomelo_platform_task_main_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);
    assert(uv_loop != NULL);

    pomelo_platform_task_main_controller_t * controller =
        pomelo_allocator_malloc_t(
            allocator,
            pomelo_platform_task_main_controller_t
        );
    if (!controller) {
        return NULL;
    }
    memset(controller, 0, sizeof(pomelo_platform_task_main_controller_t));
    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    pomelo_atomic_int64_store(&controller->running, false);

    // Create tasks pool
    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_main_t);
    pool_options.zero_initialized = true;
    pool_options.synchronized = true;
    controller->task_pool = pomelo_pool_create(&pool_options);
    if (!controller->task_pool) {
        pomelo_platform_task_main_controller_destroy(controller);
        return NULL;
    }

    // Create tasks lists
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_main_t *);
    controller->tasks_front = pomelo_list_create(&list_options);
    if (!controller->tasks_front) {
        pomelo_platform_task_main_controller_destroy(controller);
        return NULL;
    }

    controller->tasks_back = pomelo_list_create(&list_options);
    if (!controller->tasks_back) {
        pomelo_platform_task_main_controller_destroy(controller);
        return NULL;
    }

    return controller;
}


void pomelo_platform_task_main_controller_destroy(
    pomelo_platform_task_main_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_allocator_t * allocator = controller->allocator;

    if (controller->task_pool) {
        pomelo_pool_destroy(controller->task_pool);
        controller->task_pool = NULL;
    }

    if (controller->tasks_front) {
        pomelo_list_destroy(controller->tasks_front);
        controller->tasks_front = NULL;
    }

    if (controller->tasks_back) {
        pomelo_list_destroy(controller->tasks_back);
        controller->tasks_back = NULL;
    }

    pomelo_allocator_free(allocator, controller);
}


void pomelo_platform_task_main_controller_startup(
    pomelo_platform_task_main_controller_t * controller
) {
    assert(controller != NULL);
    if (!pomelo_atomic_int64_compare_exchange(
        &controller->running, false, true
    )) {
        return;
    }

    // Initialize mutex
    uv_mutex_init_recursive(&controller->mutex);

    // Initialize async
    uv_async_t * async = &controller->uv_async;
    uv_async_init(
        controller->uv_loop,
        async,
        pomelo_platform_task_main_async_callback
    );
    async->data = controller;
}


void pomelo_platform_task_main_controller_shutdown(
    pomelo_platform_task_main_controller_t * controller
) {
    assert(controller != NULL);
    if (!pomelo_atomic_int64_compare_exchange(
        &controller->running, true, false
    )) {
        return;
    }

    // Destroy mutex
    uv_mutex_destroy(&controller->mutex);

    // Close async
    uv_close((uv_handle_t *) &controller->uv_async, NULL);
}


void pomelo_platform_task_main_controller_statistic(
    pomelo_platform_task_main_controller_t * controller,
    pomelo_statistic_platform_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);
    statistic->main_tasks = controller->tasks_front->size;
}


int pomelo_platform_task_main_controller_submit(
    pomelo_platform_task_main_controller_t * controller,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(controller != NULL);
    assert(callback != NULL);

    if (!pomelo_atomic_int64_load(&controller->running)) {
        return -1; // Controller is not running
    }

    // Acquire new task
    pomelo_platform_task_main_t * task =
        pomelo_pool_acquire(controller->task_pool);
    if (!task) {
        return -1; // Failed to acquire new task
    }
    task->controller = controller;
    task->callback = callback;
    task->callback_data = callback_data;
    
    uv_mutex_t * mutex = &controller->mutex;

    /* -------------------------  Critical Begin  ----------------------- */
    uv_mutex_lock(mutex);

    pomelo_list_t * tasks = controller->tasks_front;
    pomelo_list_node_t * node = pomelo_list_push_back(tasks, task);

    uv_mutex_unlock(mutex);
    /* --------------------------  Critical End  ------------------------ */

    if (!node) {
        pomelo_platform_task_main_release(task);
        return -1; // Failed to append to list
    }

    // Send signal
    int ret = uv_async_send(&controller->uv_async);
    if (ret < 0) {
        pomelo_platform_task_main_release(task);
        return -1; // Failed to send signal
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


void pomelo_platform_task_main_release(pomelo_platform_task_main_t * task) {
    pomelo_pool_release(task->controller->task_pool, task);
}


void pomelo_platform_task_main_async_callback(uv_async_t * async) {
    assert(async != NULL);
    pomelo_platform_task_main_controller_t * controller = async->data;

    pomelo_pool_t * pool = controller->task_pool;
    uv_mutex_t * mutex = &controller->mutex;
    pomelo_list_t * tasks = controller->tasks_front;

    /* ----------- Begin mutex scope ----------- */
    uv_mutex_lock(mutex);

    // Swap the tasks lists
    controller->tasks_front = controller->tasks_back;
    controller->tasks_back = tasks;

    uv_mutex_unlock(mutex);
    /* ------------ End mutex scope ------------ */

    // Execute tasks (Out of mutex scope)

    pomelo_platform_task_main_t * task = NULL;
    while (pomelo_list_pop_front(tasks, &task) == 0) {
        task->callback(task->callback_data);
        pomelo_pool_release(pool, task);
    }
}
