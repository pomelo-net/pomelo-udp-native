#include <assert.h>
#include <string.h>
#include "executor.h"


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


pomelo_platform_threadsafe_controller_t *
pomelo_platform_threadsafe_controller_create(
    pomelo_platform_uv_t * platform,
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);
    assert(uv_loop != NULL);

    pomelo_platform_threadsafe_controller_t * controller =
        pomelo_allocator_malloc_t(
            allocator,
            pomelo_platform_threadsafe_controller_t
        );
    if (!controller) {
        return NULL;
    }
    memset(controller, 0, sizeof(pomelo_platform_threadsafe_controller_t));
    controller->platform = platform;
    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    pomelo_atomic_int64_store(&controller->running, false);
    pomelo_atomic_uint64_store(&controller->task_counter, 0);

    // Create tasks pool
    pomelo_pool_root_options_t pool_options;
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_threadsafe_t);
    pool_options.zero_init = true;
    pool_options.synchronized = true;
    controller->task_pool = pomelo_pool_root_create(&pool_options);
    if (!controller->task_pool) {
        pomelo_platform_threadsafe_controller_destroy(controller);
        return NULL;
    }

    // Create executor pool
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_threadsafe_executor_t);
    pool_options.synchronized = true;
    pool_options.alloc_data = controller;
    pool_options.on_alloc = (pomelo_pool_alloc_cb)
        pomelo_threadsafe_executor_on_alloc;
    pool_options.on_free = (pomelo_pool_free_cb)
        pomelo_threadsafe_executor_on_free;
    controller->executor_pool = pomelo_pool_root_create(&pool_options);
    if (!controller->executor_pool) {
        pomelo_platform_threadsafe_controller_destroy(controller);
        return NULL;
    }

    // Create executors list
    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_threadsafe_executor_t *),
        .synchronized = true
    };
    controller->executors = pomelo_list_create(&list_options);
    if (!controller->executors) {
        pomelo_platform_threadsafe_controller_destroy(controller);
        return NULL;
    }
    return controller;
}


void pomelo_platform_threadsafe_controller_destroy(
    pomelo_platform_threadsafe_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_allocator_t * allocator = controller->allocator;

    if (controller->task_pool) {
        pomelo_pool_destroy(controller->task_pool);
        controller->task_pool = NULL;
    }

    if (controller->executor_pool) {
        pomelo_pool_destroy(controller->executor_pool);
        controller->executor_pool = NULL;
    }

    if (controller->executors) {
        pomelo_list_destroy(controller->executors);
        controller->executors = NULL;
    }

    pomelo_allocator_free(allocator, controller);
}


void pomelo_platform_threadsafe_controller_startup(
    pomelo_platform_threadsafe_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_atomic_int64_store(&controller->running, true);
}


void pomelo_platform_threadsafe_controller_shutdown(
    pomelo_platform_threadsafe_controller_t * controller
) {
    assert(controller != NULL);
    if (!pomelo_atomic_int64_compare_exchange(
        &controller->running, /* expected */ true, /* desired */ false
    )) {
        return; // Controller is already shutting down
    }

    pomelo_list_t * executors = controller->executors;
    if (executors->size == 0) {
        pomelo_platform_threadsafe_controller_on_shutdown(controller);
        return;
    }

    pomelo_threadsafe_executor_t * executor = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, executors);
    while (pomelo_list_iterator_next(&it, &executor) == 0) {
        pomelo_threadsafe_executor_shutdown(executor, controller);
    }
}


void pomelo_platform_threadsafe_controller_statistic(
    pomelo_platform_threadsafe_controller_t * controller,
    pomelo_statistic_platform_uv_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);
    statistic->threadsafe_tasks =
        pomelo_atomic_uint64_load(&controller->task_counter);
}


pomelo_threadsafe_executor_t * pomelo_platform_uv_acquire_threadsafe_executor(
    pomelo_platform_uv_t * platform
) {
    assert(platform != NULL);
    pomelo_platform_threadsafe_controller_t * controller =
        platform->threadsafe_controller;

    if (!pomelo_atomic_int64_load(&controller->running)) {
        return NULL; // Controller is not running
    }

    pomelo_threadsafe_executor_t * executor =
        pomelo_pool_acquire(controller->executor_pool, NULL);
    if (!executor) return NULL; // Failed to acquire executor

    executor->entry = pomelo_list_push_back(controller->executors, executor);
    if (!executor->entry) {
        pomelo_pool_release(controller->executor_pool, executor);
        return NULL; // Failed to add executor to list
    }

    // Startup the executor
    int ret = pomelo_threadsafe_executor_startup(executor, controller);
    if (ret < 0) {
        pomelo_list_remove(controller->executors, executor->entry);
        pomelo_pool_release(controller->executor_pool, executor);
        return NULL; // Failed to startup executor
    }

    return executor;
}


/// @brief Release the threadsafe executor
static void release_threadsafe_executor(
    pomelo_threadsafe_executor_t * executor
) {
    assert(executor != NULL);
    pomelo_platform_threadsafe_controller_t * controller = executor->controller;
    if (!pomelo_atomic_int64_load(&controller->running)) {
        return; // Controller is not running
    }
    pomelo_threadsafe_executor_shutdown(executor, controller);
}


void pomelo_platform_uv_release_threadsafe_executor(
    pomelo_platform_uv_t * platform,
    pomelo_threadsafe_executor_t * executor
) {
    assert(platform != NULL);
    assert(executor != NULL);
    pomelo_platform_threadsafe_controller_t * controller = executor->controller;
    if (!pomelo_atomic_int64_load(&controller->running)) {
        return; // Controller is not running
    }
    pomelo_threadsafe_executor_uv_submit(
        platform,
        executor,
        (pomelo_platform_task_entry) release_threadsafe_executor,
        executor
    );
}


pomelo_platform_task_t * pomelo_threadsafe_executor_uv_submit(
    pomelo_platform_uv_t * platform,
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_task_entry entry,
    void * data
) {
    assert(platform != NULL);
    assert(executor != NULL);
    assert(entry != NULL);

    pomelo_platform_threadsafe_controller_t * controller = executor->controller;
    if (!pomelo_atomic_int64_load(&controller->running)) {
        return NULL; // Controller is not running
    }

    if (!pomelo_atomic_int64_load(&executor->running)) {
        return NULL; // Executor is not running
    }

    // Acquire new task
    pomelo_platform_task_threadsafe_t * task =
        pomelo_pool_acquire(controller->task_pool, NULL);
    if (!task) {
        return NULL; // Failed to acquire new task
    }
    task->controller = controller;
    task->entry = entry;
    task->data = data;
    
    uv_mutex_t * mutex = &executor->mutex;

    /* -------------------------  Critical Begin  ----------------------- */
    uv_mutex_lock(mutex);

    pomelo_list_t * tasks = executor->tasks_front;
    pomelo_list_entry_t * list_entry = pomelo_list_push_back(tasks, task);

    uv_mutex_unlock(mutex);
    /* --------------------------  Critical End  ------------------------ */

    if (!list_entry) {
        pomelo_platform_task_threadsafe_release(task);
        return NULL; // Failed to append to list
    }

    // Send signal
    int ret = uv_async_send(&executor->uv_async);
    if (ret < 0) {
        pomelo_platform_task_threadsafe_release(task);
        return NULL; // Failed to send signal
    }

    pomelo_atomic_uint64_fetch_add(&controller->task_counter, 1);
    return (pomelo_platform_task_t *) task;
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */


void pomelo_platform_task_threadsafe_release(
    pomelo_platform_task_threadsafe_t * task
) {
    assert(task != NULL);
    pomelo_pool_release(task->controller->task_pool, task);
}


void pomelo_platform_task_threadsafe_async_callback(uv_async_t * async) {
    assert(async != NULL);
    pomelo_threadsafe_executor_t * executor = async->data;

    // Set the busy flag
    executor->flags |= POMELO_EXECUTOR_FLAG_BUSY;

    uv_mutex_t * mutex = &executor->mutex;
    pomelo_list_t * tasks = executor->tasks_front;

    /* ----------- Begin mutex scope ----------- */
    uv_mutex_lock(mutex);

    // Swap the tasks lists
    executor->tasks_front = executor->tasks_back;
    executor->tasks_back = tasks;

    uv_mutex_unlock(mutex);
    /* ------------ End mutex scope ------------ */

    // Update task counter
    pomelo_platform_threadsafe_controller_t * controller = executor->controller;
    pomelo_atomic_uint64_fetch_sub(&controller->task_counter, tasks->size);

    // Execute tasks
    pomelo_platform_task_threadsafe_t * task = NULL;
    pomelo_platform_task_entry entry = NULL;
    void * data = NULL;
    while (
        !(executor->flags & POMELO_EXECUTOR_FLAG_SHUTDOWN) &&
        pomelo_list_pop_front(tasks, &task) == 0
    ) {
        entry = task->entry;
        data = task->data;
        pomelo_platform_task_threadsafe_release(task);

        entry(data);
    }

    // Clear the busy flag
    executor->flags &= ~POMELO_EXECUTOR_FLAG_BUSY;
    if (executor->flags & POMELO_EXECUTOR_FLAG_SHUTDOWN) {
        pomelo_threadsafe_executor_shutdown(executor, executor->controller);
    }
}


int pomelo_threadsafe_executor_on_alloc(
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_threadsafe_controller_t * controller
) {
    assert(executor != NULL);
    assert(controller != NULL);

    executor->controller = controller;
    pomelo_atomic_int64_store(&executor->running, false);

    // Create tasks lists
    pomelo_list_options_t list_options = {
        .allocator = controller->allocator,
        .element_size = sizeof(pomelo_platform_task_threadsafe_t *)
    };
    executor->tasks_front = pomelo_list_create(&list_options);
    if (!executor->tasks_front) {
        return -1;
    }

    executor->tasks_back = pomelo_list_create(&list_options);
    if (!executor->tasks_back) {
        return -1;
    }

    // Initialize mutex
    uv_mutex_init_recursive(&executor->mutex);
    return 0;
}


void pomelo_threadsafe_executor_on_free(
    pomelo_threadsafe_executor_t * executor
) {
    assert(executor != NULL);

    if (executor->tasks_front) {
        pomelo_list_destroy(executor->tasks_front);
        executor->tasks_front = NULL;
    }

    if (executor->tasks_back) {
        pomelo_list_destroy(executor->tasks_back);
        executor->tasks_back = NULL;
    }

    uv_mutex_destroy(&executor->mutex);
}


int pomelo_threadsafe_executor_startup(
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_threadsafe_controller_t * controller
) {
    assert(executor != NULL);
    (void) controller;

    // Initialize async
    uv_async_t * async = &executor->uv_async;
    int ret = uv_async_init(
        controller->uv_loop,
        async,
        pomelo_platform_task_threadsafe_async_callback
    );
    if (ret < 0) {
        return -1;
    }
    async->data = executor;

    pomelo_atomic_int64_store(&executor->running, true);
    return 0;
}


/// @brief Callback for when the async handle is closed
static void executor_shutdown_complete(uv_handle_t * handle) {
    pomelo_threadsafe_executor_t * executor = handle->data;
    pomelo_platform_threadsafe_controller_t * controller = executor->controller;
    pomelo_pool_release(controller->executor_pool, executor);

    assert(executor->entry != NULL);
    pomelo_list_remove(controller->executors, executor->entry);
    executor->entry = NULL;

    if (
        !pomelo_atomic_int64_load(&controller->running) &&
        controller->executors->size == 0
    ) {
        pomelo_platform_threadsafe_controller_on_shutdown(controller);
    }
}


int pomelo_threadsafe_executor_shutdown(
    pomelo_threadsafe_executor_t * executor,
    pomelo_platform_threadsafe_controller_t * controller
) {
    assert(executor != NULL);
    assert(controller != NULL);

    if (executor->flags & POMELO_EXECUTOR_FLAG_BUSY) {
        // Just set the flag for later shutdown
        executor->flags |= POMELO_EXECUTOR_FLAG_SHUTDOWN;
        return 0; // Executor is busy
    }

    if (!pomelo_atomic_int64_compare_exchange(
        &executor->running, /* expected */ true, /* desired */ false
    )) {
        assert(false);
        return -1; // Executor is not running
    }

    uv_async_t * async = &executor->uv_async;
    uv_close((uv_handle_t *) async, executor_shutdown_complete);

    // Clean all tasks
    uv_mutex_t * mutex = &executor->mutex;

    /* ----------- Begin mutex scope ----------- */
    uv_mutex_lock(mutex);

    // Update task counter
    pomelo_atomic_uint64_fetch_sub(
        &controller->task_counter,
        executor->tasks_front->size + executor->tasks_back->size
    );

    pomelo_platform_task_threadsafe_t * task = NULL;
    pomelo_list_t * tasks = executor->tasks_front;
    while (pomelo_list_pop_front(tasks, &task) == 0) {
        pomelo_platform_task_threadsafe_release(task);
    }

    tasks = executor->tasks_back;
    while (pomelo_list_pop_front(tasks, &task) == 0) {
        pomelo_platform_task_threadsafe_release(task);
    }

    uv_mutex_unlock(mutex);
    /* ------------ End mutex scope ------------ */

    return 0;
}
