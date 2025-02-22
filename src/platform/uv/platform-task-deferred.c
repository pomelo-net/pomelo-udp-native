#include <assert.h>
#include <string.h>
#include "platform-task-deferred.h"
#include "platform-task-group.h"


/* -------------------------------------------------------------------------- */
/*                               Internal APIs                                */
/* -------------------------------------------------------------------------- */

pomelo_platform_task_deferred_controller_t *
pomelo_platform_task_deferred_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);

    pomelo_platform_task_deferred_controller_t * controller =
        pomelo_allocator_malloc_t(
            allocator,
            pomelo_platform_task_deferred_controller_t
        );
    if (!controller) {
        return NULL;
    }

    memset(controller, 0, sizeof(pomelo_platform_task_deferred_controller_t));
    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.zero_initialized = true;
    pool_options.element_size = sizeof(pomelo_platform_task_deferred_t);

    controller->task_pool = pomelo_pool_create(&pool_options);
    if (!controller->task_pool) {
        pomelo_platform_task_deferred_controller_destroy(controller);
        return NULL;
    }

    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_deferred_t *);
    controller->tasks = pomelo_list_create(&list_options);
    if (!controller->tasks) {
        pomelo_platform_task_deferred_controller_destroy(controller);
        return NULL;
    }

    return controller;
}


void pomelo_platform_task_deferred_controller_destroy(
    pomelo_platform_task_deferred_controller_t * controller
) {
    assert(controller != NULL);
    pomelo_allocator_t * allocator = controller->allocator;

    if (controller->task_pool) {
        pomelo_pool_destroy(controller->task_pool);
        controller->task_pool = NULL;
    }

    if (controller->tasks) {
        pomelo_list_destroy(controller->tasks);
        controller->tasks = NULL;
    }

    pomelo_allocator_free(allocator, controller);
}


void pomelo_platform_task_deferred_controller_startup(
    pomelo_platform_task_deferred_controller_t * controller
) {
    assert(controller != NULL);
    controller->running = true;
}


void pomelo_platform_task_deferred_controller_shutdown(
    pomelo_platform_task_deferred_controller_t * controller
) {
    assert(controller != NULL);
    if (!controller->running) {
        return;
    }
    controller->running = false;

    pomelo_platform_task_deferred_t * task = NULL;
    pomelo_list_for(
        controller->tasks,
        task,
        pomelo_platform_task_deferred_t *,
        { pomelo_platform_task_deferred_cancel(task); }
    );
}


void pomelo_platform_task_deferred_controller_statistic(
    pomelo_platform_task_deferred_controller_t * controller,
    pomelo_statistic_platform_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);

    statistic->deferred_tasks = controller->tasks->size;
}


void pomelo_platform_task_deferred_callback(uv_idle_t * idle) {
    pomelo_platform_task_deferred_t * task = idle->data;
    pomelo_platform_task_cb callback = task->callback;
    void * callback_data = task->callback_data;
    bool canceled = task->canceled;

    uv_idle_stop(idle);
    pomelo_platform_task_deferred_release(task);

    // Only call the callback if task is not canceled
    if (!canceled) {
        callback(callback_data);
    }
}


void pomelo_platform_task_deferred_release(
    pomelo_platform_task_deferred_t * task
) {
    assert(task != NULL);
    pomelo_platform_task_deferred_controller_t * controller = task->controller;
    pomelo_list_remove(controller->tasks, task->global_node);
    if (task->group_node) {
        pomelo_list_remove(task->group->deferred_tasks, task->group_node);
        task->group_node = NULL;
    }
    pomelo_pool_release(controller->task_pool, task);
}


void pomelo_platform_task_deferred_cancel(
    pomelo_platform_task_deferred_t * task
) {
    assert(task != NULL);
    task->canceled = true;
}


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

int pomelo_platform_task_deferred_controller_submit(
    pomelo_platform_task_deferred_controller_t * controller,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(controller != NULL);
    assert(callback != NULL);
    
    if (!controller->running) {
        return -1; // Controller has not been started up
    }

    if (group && group->cancel_callback) {
        return -1; // Group is canceling
    }

    pomelo_platform_task_deferred_t * task =
        pomelo_pool_acquire(controller->task_pool);
    if (!task) {
        return -1; // Failed to acquire new task
    }

    uv_idle_t * idle = &task->idle;
    uv_idle_init(controller->uv_loop, idle);
    idle->data = task;

    task->controller = controller;
    task->callback = callback;
    task->callback_data = callback_data;
    task->group = group;

    if (group) {
        task->group_node = pomelo_list_push_back(group->deferred_tasks, task);
        if (!task->group_node) {
            pomelo_platform_task_deferred_release(task);
            return -1;
        }
    }

    task->global_node = pomelo_list_push_back(controller->tasks, task);
    if (!task->global_node) {
        pomelo_platform_task_deferred_release(task);
        return -1;
    }

    int ret = uv_idle_start(idle, pomelo_platform_task_deferred_callback);
    if (ret < 0) {
        pomelo_platform_task_deferred_release(task);
        return -1; // Failed to start
    }

    return 0;
}
