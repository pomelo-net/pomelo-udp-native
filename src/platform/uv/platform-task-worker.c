#include <assert.h>
#include <string.h>
#include "platform-task-worker.h"
#include "platform-task-group.h"


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

pomelo_platform_task_worker_controller_t *
pomelo_platform_task_worker_controller_create(
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);
    assert(uv_loop != NULL);

    pomelo_platform_task_worker_controller_t * controller =
        pomelo_allocator_malloc_t(
            allocator,
            pomelo_platform_task_worker_controller_t
        );
    if (!controller) {
        return NULL;
    }
    memset(controller, 0, sizeof(pomelo_platform_task_worker_controller_t));

    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    pomelo_pool_options_t pool_options;
    pomelo_pool_options_init(&pool_options);
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_platform_task_worker_t);
    pool_options.zero_initialized = true;
    controller->task_pool = pomelo_pool_create(&pool_options);
    if (!controller->task_pool) {
        pomelo_platform_task_worker_controller_destroy(controller);
        return NULL;
    }

    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_worker_t *);
    controller->tasks = pomelo_list_create(&list_options);
    if (!controller->tasks) {
        pomelo_platform_task_worker_controller_destroy(controller);
        return NULL;
    }

    return controller;
}


void pomelo_platform_task_worker_controller_destroy(
    pomelo_platform_task_worker_controller_t * controller
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


void pomelo_platform_task_worker_controller_startup(
    pomelo_platform_task_worker_controller_t * controller
) {
    assert(controller != NULL);
    controller->running = true;
}


void pomelo_platform_task_worker_controller_shutdown(
    pomelo_platform_task_worker_controller_t * controller
) {
    assert(controller != NULL);
    if (!controller->running) {
        return;
    }

    pomelo_platform_task_worker_t * task = NULL;
    pomelo_list_for(controller->tasks, task, pomelo_platform_task_worker_t *, {
        pomelo_platform_task_worker_cancel(task, NULL, NULL);
    });
}


void pomelo_platform_task_worker_controller_statistic(
    pomelo_platform_task_worker_controller_t * controller,
    pomelo_statistic_platform_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);
    statistic->worker_tasks = controller->tasks->size;
}


void pomelo_platform_task_worker_cancel(
    pomelo_platform_task_worker_t * task,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(task != NULL);
    if (task->canceled) return;
    
    // Try to cancel task
    task->canceled = true;
    task->cancel_callback = callback;
    task->cancel_callback_data = callback_data;
    uv_cancel((uv_req_t *) &task->uv_work);
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_platform_task_worker_entry(uv_work_t * uv_work) {
    pomelo_platform_task_worker_t * task = uv_work->data;
    task->entry(task->callback_data);
}


void pomelo_platform_task_worker_done(uv_work_t * uv_work, int status) {
    pomelo_platform_task_worker_t * task = uv_work->data;
    bool canceled = (status == UV_ECANCELED || task->canceled);
    pomelo_platform_task_done_cb callback = task->done;
    void * callback_data = task->callback_data;
    pomelo_platform_task_cb cancel_callback = task->cancel_callback;
    void * cancel_callback_data = task->cancel_callback_data;

    pomelo_platform_task_worker_release(task);

    // Worker tasks eventually will be done.
    callback(callback_data, canceled);

    if (canceled && cancel_callback) {
        cancel_callback(cancel_callback_data);
    }
}


void pomelo_platform_task_worker_release(pomelo_platform_task_worker_t * task) {
    assert(task != NULL);

    pomelo_platform_task_worker_controller_t * controller = task->controller;
    pomelo_list_remove(controller->tasks, task->global_node);
    if (task->group_node) {
        pomelo_list_remove(task->group->worker_tasks, task->group_node);
        task->group_node = NULL;
    }
    pomelo_pool_release(controller->task_pool, task);
}


int pomelo_platform_task_worker_controller_submit(
    pomelo_platform_task_worker_controller_t * controller,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb entry,
    pomelo_platform_task_done_cb done,
    void * callback_data
) {
    assert(controller != NULL);
    assert(entry != NULL);

    if (!controller->running) {
        return -1; // Controller is not running
    }

    if (group && group->cancel_callback) {
        return -1; // Group is canceling, add task it not permitted
    }

    pomelo_platform_task_worker_t * task =
        pomelo_pool_acquire(controller->task_pool);
    if (!task) {
        return -1; // Failed to acquire new task
    }

    task->controller = controller;
    task->entry = entry;
    task->done = done;
    task->callback_data = callback_data;
    task->group = group;

    task->global_node = pomelo_list_push_back(controller->tasks, task);
    if (!task->global_node) {
        pomelo_platform_task_worker_release(task);
        return -1; // Failed to append to global list
    }

    if (group) {
        task->group_node = pomelo_list_push_back(group->worker_tasks, task);
        if (!task->group_node) {
            pomelo_platform_task_worker_release(task);
            return -1; // Failed to append to group list
        }
    }

    // Setup UV work
    uv_work_t * work = &task->uv_work;
    work->data = task;

    int ret = uv_queue_work(
        controller->uv_loop,
        work,
        pomelo_platform_task_worker_entry,
        pomelo_platform_task_worker_done
    );
    if (ret < 0) {
        pomelo_platform_task_worker_release(task);
        return -1; // Failed to queue work
    }

    return 0;
}
