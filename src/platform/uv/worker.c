#include <assert.h>
#include <string.h>
#include "worker.h"


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */

pomelo_platform_worker_controller_t * pomelo_platform_worker_controller_create(
    pomelo_platform_uv_t * platform,
    pomelo_allocator_t * allocator,
    uv_loop_t * uv_loop
) {
    assert(allocator != NULL);
    assert(uv_loop != NULL);

    pomelo_platform_worker_controller_t * controller =
        pomelo_allocator_malloc_t(
            allocator, pomelo_platform_worker_controller_t
        );
    if (!controller) return NULL;
    memset(controller, 0, sizeof(pomelo_platform_worker_controller_t));
    controller->platform = platform;
    controller->allocator = allocator;
    controller->uv_loop = uv_loop;

    pomelo_pool_root_options_t pool_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_platform_task_worker_t),
        .zero_init = true
    };
    controller->task_pool = pomelo_pool_root_create(&pool_options);
    if (!controller->task_pool) {
        pomelo_platform_worker_controller_destroy(controller);
        return NULL;
    }

    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_platform_task_worker_t *)
    };
    controller->tasks = pomelo_list_create(&list_options);
    if (!controller->tasks) {
        pomelo_platform_worker_controller_destroy(controller);
        return NULL;
    }

    return controller;
}


void pomelo_platform_worker_controller_destroy(
    pomelo_platform_worker_controller_t * controller
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


void pomelo_platform_worker_controller_startup(
    pomelo_platform_worker_controller_t * controller
) {
    assert(controller != NULL);
    controller->running = true;
}


void pomelo_platform_worker_controller_shutdown(
    pomelo_platform_worker_controller_t * controller
) {
    assert(controller != NULL);
    if (!controller->running) {
        return; // Controller is already shutting down
    }
    controller->running = false;
    if (controller->tasks->size == 0) {
        pomelo_platform_worker_controller_on_shutdown(controller);
        return;
    }

    pomelo_platform_task_worker_t * task = NULL;
    pomelo_list_iterator_t it;
    pomelo_list_iterator_init(&it, controller->tasks);
    while (pomelo_list_iterator_next(&it, &task) == 0) {
        pomelo_platform_cancel_worker_task_ex(task);
    }
}


void pomelo_platform_worker_controller_statistic(
    pomelo_platform_worker_controller_t * controller,
    pomelo_statistic_platform_uv_t * statistic
) {
    assert(controller != NULL);
    assert(statistic != NULL);
    statistic->worker_tasks = controller->tasks->size;
}


pomelo_platform_task_t * pomelo_platform_uv_submit_worker_task(
    pomelo_platform_uv_t * platform,
    pomelo_platform_task_entry entry,
    pomelo_platform_task_complete complete,
    void * data
) {
    assert(platform != NULL);
    assert(entry != NULL);

    pomelo_platform_worker_controller_t * controller =
        platform->worker_controller;
    if (!controller->running) {
        return NULL; // Controller is not running
    }

    pomelo_platform_task_worker_t * task =
        pomelo_pool_acquire(controller->task_pool, NULL);
    if (!task) {
        return NULL; // Failed to acquire new task
    }

    task->controller = controller;
    task->entry = entry;
    task->complete = complete;
    task->data = data;

    task->list_entry = pomelo_list_push_back(controller->tasks, task);
    if (!task->list_entry) {
        pomelo_platform_worker_release(task);
        return NULL; // Failed to append to global list
    }

    // Setup UV work
    uv_work_t * work = &task->uv_work;
    work->data = task;

    int ret = uv_queue_work(
        controller->uv_loop,
        work,
        pomelo_platform_worker_entry,
        pomelo_platform_worker_done
    );
    if (ret < 0) {
        pomelo_platform_worker_release(task);
        return NULL; // Failed to queue work
    }

    return (pomelo_platform_task_t *) task;
}


void pomelo_platform_uv_cancel_worker_task(
    pomelo_platform_uv_t * platform,
    pomelo_platform_task_t * task
) {
    (void) platform;
    pomelo_platform_cancel_worker_task_ex(
        (pomelo_platform_task_worker_t *) task
    );
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */

void pomelo_platform_worker_entry(uv_work_t * uv_work) {
    pomelo_platform_task_worker_t * task = uv_work->data;
    task->entry(task->data);
}


void pomelo_platform_worker_done(uv_work_t * uv_work, int status) {
    pomelo_platform_task_worker_t * task = uv_work->data;
    bool canceled = (status == UV_ECANCELED || task->canceled);
    pomelo_platform_task_complete complete = task->complete;
    void * data = task->data;

    pomelo_platform_worker_release(task);

    // Worker tasks eventually will be done.
    complete(data, canceled);

    pomelo_platform_worker_controller_t * controller = task->controller;
    if (!controller->running && controller->tasks->size == 0) {
        pomelo_platform_worker_controller_on_shutdown(controller);
    }
}


void pomelo_platform_worker_release(pomelo_platform_task_worker_t * task) {
    assert(task != NULL);

    pomelo_platform_worker_controller_t * controller = task->controller;
    pomelo_list_remove(controller->tasks, task->list_entry);
    pomelo_pool_release(controller->task_pool, task);
}


void pomelo_platform_cancel_worker_task_ex(
    pomelo_platform_task_worker_t * task
) {
    assert(task != NULL);
    if (task->canceled) {
        return;
    }
    
    task->canceled = true;
    uv_cancel((uv_req_t *) &task->uv_work);
}
