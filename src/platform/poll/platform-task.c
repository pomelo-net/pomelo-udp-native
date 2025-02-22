#include <assert.h>
#include <string.h>
#include "platform-task.h"


pomelo_platform_task_controller_t * pomelo_platform_task_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);
    assert(uv_loop != NULL);

    pomelo_platform_task_controller_t * controller =
        pomelo_allocator_malloc_t(allocator, pomelo_platform_task_controller_t);
    if (!controller) {
        return NULL;
    }
    memset(controller, 0, sizeof(pomelo_platform_task_controller_t));

    controller->allocator = allocator;
    controller->uv_loop = uv_loop;
    pomelo_atomic_int64_store(&controller->running, false);
    pomelo_atomic_int64_store(&controller->main_tasks_available, false);

    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_late_t);
    pool_options.zero_initialized = true;
    controller->task_late_pool = pomelo_pool_create(&pool_options);
    if (!controller->task_late_pool) {
        pomelo_platform_task_controller_destroy(controller);
        return NULL;
    }

    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_main_t);
    pool_options.zero_initialized = true;
    pool_options.synchronized = true;
    controller->task_main_pool = pomelo_pool_create(&pool_options);
    if (!controller->task_main_pool) {
        pomelo_platform_task_controller_destroy(controller);
        return NULL;
    }

    // Create list of tasks
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_main_t *);
    controller->main_tasks_front = pomelo_list_create(&list_options);
    if (!controller->main_tasks_front) {
        pomelo_platform_task_controller_destroy(controller);
        return NULL;
    }

    controller->main_tasks_back = pomelo_list_create(&list_options);
    if (!controller->main_tasks_back) {
        pomelo_platform_task_controller_destroy(controller);
        return NULL;
    }

    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_late_t *);
    controller->late_tasks_front = pomelo_list_create(&list_options);
    if (!controller->late_tasks_front) {
        pomelo_platform_task_controller_destroy(controller);
        return NULL;
    }

    controller->late_tasks_back = pomelo_list_create(&list_options);
    if (!controller->late_tasks_back) {
        pomelo_platform_task_controller_destroy(controller);
        return NULL;
    }

    return controller;
}


/// @brief Destroy task controller
void pomelo_platform_task_controller_destroy(
    pomelo_platform_task_controller_t * controller
) {
    assert(controller != NULL);
    
    if (controller->task_late_pool) {
        pomelo_pool_destroy(controller->task_late_pool);
        controller->task_late_pool = NULL;
    }

    if (controller->task_main_pool) {
        pomelo_pool_destroy(controller->task_main_pool);
        controller->task_main_pool = NULL;
    }

    if (controller->main_tasks_front) {
        pomelo_list_destroy(controller->main_tasks_front);
        controller->main_tasks_front = NULL;
    }

    if (controller->main_tasks_back) {
        pomelo_list_destroy(controller->main_tasks_back);
        controller->main_tasks_back = NULL;
    }

    if (controller->late_tasks_front) {
        pomelo_list_destroy(controller->late_tasks_front);
        controller->late_tasks_front = NULL;
    }

    if (controller->late_tasks_back) {
        pomelo_list_destroy(controller->late_tasks_back);
        controller->late_tasks_back = NULL;
    }

    pomelo_allocator_free(controller->allocator, controller);
}


void pomelo_platform_task_controller_startup(
    pomelo_platform_task_controller_t * controller
) {
    assert(controller != NULL);
    if (!pomelo_atomic_int64_compare_exchange(
        &controller->running, false, true
    )) {
        return; // Controller has been started up
    }

    uv_mutex_init_recursive(&controller->uv_mutex);
}


void pomelo_platform_task_controller_shutdown(
    pomelo_platform_task_controller_t * controller
) {
    assert(controller != NULL);

    if (!pomelo_atomic_int64_compare_exchange(
        &controller->running, true, false
    )) {
        return;
    }


    uv_mutex_t * mutex = &controller->uv_mutex;
    pomelo_list_t * tasks = controller->main_tasks_front;

    /* Critical section begin */
    uv_mutex_lock(mutex);

    controller->main_tasks_front = controller->main_tasks_back;
    controller->main_tasks_back = tasks;

    uv_mutex_unlock(mutex);
    /* Critical section begin */

    pomelo_platform_task_main_t * task_main = NULL;
    pomelo_pool_t * task_main_pool = controller->task_main_pool;
    while (pomelo_list_pop_front(tasks, &task_main) == 0) {
        pomelo_pool_release(task_main_pool, task_main);
    }

    tasks = controller->late_tasks_front;
    pomelo_platform_task_late_t * task_late = NULL;
    pomelo_pool_t * task_late_pool = controller->task_late_pool;
    while (pomelo_list_pop_front(tasks, &task_late) == 0) {
        pomelo_pool_release(task_late_pool, task_late);
    }

    uv_mutex_destroy(&controller->uv_mutex);
}


void pomelo_platform_task_controller_statistic(
    pomelo_platform_task_controller_t * controller,
    pomelo_statistic_platform_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);

    statistic->main_tasks = controller->main_tasks_front->size;
    statistic->deferred_tasks = controller->deferred_task_count;
    statistic->worker_tasks = controller->worker_task_count;
}


int pomelo_platform_task_controller_service(
    pomelo_platform_task_controller_t * controller
) {
    assert(controller != NULL);
    if (!pomelo_atomic_int64_load(&controller->running)) {
        return -1; // Platform is not running
    }

    pomelo_platform_task_controller_process_main_tasks(controller);
    pomelo_platform_task_controller_process_late_tasks(controller);

    return 0;
}


int pomelo_platform_task_controller_submit_main(
    pomelo_platform_task_controller_t * controller,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(controller != NULL);
    assert(callback != NULL);

    if (!pomelo_atomic_int64_load(&controller->running)) {
        return -1; // Platform is not running
    }

    // Create task
    pomelo_platform_task_main_t * task =
        pomelo_pool_acquire(controller->task_main_pool);
    if (!task) {
        return -1; // Failed to acquire new task
    }

    task->controller = controller;
    task->callback = callback;
    task->callback_data = callback_data;

    uv_mutex_t * mutex = &controller->uv_mutex;
    
    /* Critical section begin */
    uv_mutex_lock(mutex);

    pomelo_list_t * tasks = controller->main_tasks_front;
    task->global_node = pomelo_list_push_back(tasks, task);

    uv_mutex_unlock(mutex);
    /* Critical section end */

    if (!task->global_node) {
        pomelo_platform_task_main_release(task);
        return -1; // Failed to push the task
    }

    pomelo_atomic_int64_store(&controller->main_tasks_available, true);
    return 0;
}


int pomelo_platform_task_controller_submit_late(
    pomelo_platform_task_controller_t * controller,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb entry,
    pomelo_platform_task_done_cb done,
    void * callback_data
) {
    assert(controller != NULL);
    assert(entry != NULL);

    if (!pomelo_atomic_int64_load(&controller->running)) {
        return -1; // Platform is not running
    }

    if (group && group->cancel_callback) {
        return -1; // Group is canceling
    }

    pomelo_platform_task_late_t * task =
        pomelo_pool_acquire(controller->task_late_pool);
    if (!task) {
        return -1; // Failed to acquire new task
    }

    task->controller = controller;
    task->entry = entry;
    task->done = done;
    task->callback_data = callback_data;
    task->group = group;
    if (group) {
        task->group_node = pomelo_list_push_back(group->tasks, task);
        if (!task->group_node) {
            pomelo_platform_task_late_release(task);
            return -1; // Failed to append task to group tasks list
        }
    }

    pomelo_list_node_t * node =
        pomelo_list_push_back(controller->late_tasks_front, task);
    if (!node) {
        pomelo_platform_task_late_release(task);
        return -1; // Failed to append task to front tasks list
    }

    if (done) {
        // This is a worker task
        controller->worker_task_count++;
    } else {
        // This is a deferred task
        controller->deferred_task_count++;
    }

    return 0;
}


int pomelo_platform_task_group_init(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
) {
    assert(group != NULL);
    assert(allocator != NULL);

    memset(group, 0, sizeof(pomelo_platform_task_group_t));

    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_late_t *);
    group->tasks = pomelo_list_create(&list_options);
    if (!group->tasks) {
        pomelo_platform_task_group_finalize(group, allocator);
        return -1;
    }

    return 0;
}


int pomelo_platform_task_group_reset(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
) {
    assert(group != NULL);
    (void) allocator;
    pomelo_list_clear(group->tasks);
    group->cancel_callback = NULL;
    group->cancel_callback_data = NULL;

    return 0;
}


int pomelo_platform_task_group_finalize(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
) {
    assert(group != NULL);
    (void) allocator;
    pomelo_list_destroy(group->tasks);
    return 0;
}


int pomelo_platform_cancel_task_group(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(platform != NULL);
    assert(group != NULL);

    pomelo_platform_task_late_t * task = NULL;
    size_t worker_task_count = 0;
    pomelo_list_for(group->tasks, task, pomelo_platform_task_late_t *, {
        task->canceled = true;
        if (task->done) {
            worker_task_count++;
        }
    });

    if (worker_task_count == 0 && callback) {
        return pomelo_platform_submit_deferred_task(
            platform,
            NULL,
            callback,
            callback_data
        );
    }

    // Set callback for later calling
    group->cancel_callback = callback;
    group->cancel_callback_data = callback_data;

    return 0;
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_platform_task_main_release(pomelo_platform_task_main_t * task) {
    assert(task != NULL);
    pomelo_platform_task_controller_t * controller = task->controller;

    if (task->global_node) {
        uv_mutex_t * mutex = &controller->uv_mutex;
        uv_mutex_lock(mutex);
        /* Critical section begin */

        pomelo_list_remove(controller->main_tasks_front, task->global_node);

        /* Critical section begin */
        uv_mutex_unlock(mutex);
        task->global_node = NULL;
    }
    

    pomelo_pool_release(task->controller->task_main_pool, task);
}


void pomelo_platform_task_late_release(pomelo_platform_task_late_t * task) {
    assert(task != NULL);
    
    if (task->group_node) {
        pomelo_list_remove(task->group->tasks, task->group_node);
        task->group_node = NULL;
    }

    pomelo_pool_release(task->controller->task_late_pool, task);
}


void pomelo_platform_task_controller_process_main_tasks(
    pomelo_platform_task_controller_t * controller
) {
    assert(controller != NULL);

    if (!pomelo_atomic_int64_compare_exchange(
        &controller->main_tasks_available, true, false
    )) {
        return;
    }

    // Execute main tasks
    pomelo_list_t * tasks = controller->main_tasks_front;
    uv_mutex_t * mutex = &controller->uv_mutex;
    
    /* Critical section begin */
    uv_mutex_lock(mutex);
    if (tasks->size == 0) {
        uv_mutex_unlock(mutex);
        return; // Tasks list is empty
    }

    // Swap the lists
    controller->main_tasks_front = controller->main_tasks_back;
    controller->main_tasks_back = tasks;

    uv_mutex_unlock(mutex);
    /* Critical section end */

    pomelo_pool_t * task_main_pool = controller->task_main_pool;
    pomelo_platform_task_main_t * task = NULL;
    while (pomelo_list_pop_front(tasks, &task) == 0) {
        task->callback(task->callback_data);
        pomelo_pool_release(task_main_pool, task);
    }
}


void pomelo_platform_task_controller_process_late_tasks(
    pomelo_platform_task_controller_t * controller
) {
    assert(controller != NULL);

    // Swap the list
    pomelo_list_t * tasks = controller->late_tasks_front;
    if (tasks->size == 0) {
        return; // Tasks list is empty
    }

    controller->late_tasks_front = controller->late_tasks_back;
    controller->late_tasks_back = tasks;

    pomelo_pool_t * task_late_pool = controller->task_late_pool;
    pomelo_platform_task_late_t * task = NULL;
    while (pomelo_list_pop_front(tasks, &task) == 0) {
        void * callback_data = task->callback_data;
        bool canceled = task->canceled;

        if (!canceled) {
            task->entry(callback_data);
        }
        if (task->done) {
            task->done(callback_data, canceled);
        }

        // Check group
        pomelo_platform_task_group_t * group = task->group;
        if (group) {
            pomelo_platform_task_group_finish_task(group, task);
        }

        pomelo_pool_release(task_late_pool, task);
    }
}


void pomelo_platform_task_group_finish_task(
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_late_t * task
) {
    assert(group != NULL);
    assert(task != NULL);

    pomelo_list_remove(group->tasks, task->group_node);
    task->group_node = NULL;
    if (!group->cancel_callback || group->tasks->size > 0) {
        return;
    }

    // Call the callback
    pomelo_platform_task_cb cancel_callback =
        group->cancel_callback;
    void * cancel_callback_data = group->cancel_callback_data;
    group->cancel_callback = NULL;
    group->cancel_callback_data = NULL;

    cancel_callback(cancel_callback_data);
}
