#include <assert.h>
#include <string.h>
#include "platform-task-group.h"
#include "platform-task-deferred.h"
#include "platform-task-main.h"
#include "platform-task-worker.h"


/* -------------------------------------------------------------------------- */
/*                               Internal APIs                                */
/* -------------------------------------------------------------------------- */

int pomelo_platform_task_group_init(
    pomelo_platform_task_group_t * group,
    pomelo_allocator_t * allocator
) {
    assert(group != NULL);
    assert(allocator != NULL);

    memset(group, 0, sizeof(pomelo_platform_task_group_t));

    // Create lists
    pomelo_list_options_t list_options;
    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_deferred_t *);
    group->deferred_tasks = pomelo_list_create(&list_options);
    if (!group->deferred_tasks) {
        pomelo_platform_task_group_finalize(group, allocator);
        return -1;
    }

    pomelo_list_options_init(&list_options);
    list_options.allocator = allocator;
    list_options.element_size = sizeof(pomelo_platform_task_worker_t *);
    group->worker_tasks = pomelo_list_create(&list_options);
    if (!group->worker_tasks) {
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

    pomelo_platform_task_deferred_t * task_deferred = NULL;
    pomelo_platform_task_worker_t * task_worker = NULL;

    pomelo_list_for(
        group->deferred_tasks,
        task_deferred,
        pomelo_platform_task_deferred_t *,
        { task_deferred->group_node = NULL; }
    );

    pomelo_list_for(
        group->worker_tasks,
        task_worker,
        pomelo_platform_task_worker_t *,
        { task_worker->group_node = NULL; }
    );

    pomelo_list_clear(group->deferred_tasks);
    pomelo_list_clear(group->worker_tasks);

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

    if (group->deferred_tasks) {
        pomelo_list_destroy(group->deferred_tasks);
        group->deferred_tasks = NULL;
    }

    if (group->worker_tasks) {
        pomelo_list_destroy(group->worker_tasks);
        group->worker_tasks = NULL;
    }

    return 0;
}


void pomelo_platform_task_group_on_worker_task_canceled(
    pomelo_platform_task_group_t * group
) {
    assert(group != NULL);
    if (group->worker_tasks->size > 0) {
        return;
    }

    pomelo_platform_task_cb callback = group->cancel_callback;
    void * callback_data = group->cancel_callback_data;

    group->cancel_callback = NULL;
    group->cancel_callback_data = NULL;

    if (callback) {
        // Call the callback
        callback(callback_data);
    }
}


int pomelo_platform_cancel_task_group(
    pomelo_platform_t * platform,
    pomelo_platform_task_group_t * group,
    pomelo_platform_task_cb callback,
    void * callback_data
) {
    assert(platform != NULL);
    assert(group != NULL);

    pomelo_platform_task_deferred_t * deferred_task = NULL;
    pomelo_list_for(
        group->deferred_tasks,
        deferred_task,
        pomelo_platform_task_deferred_t *,
        { pomelo_platform_task_deferred_cancel(deferred_task); }
    );

    size_t remain_worker_tasks = group->worker_tasks->size;
    if (remain_worker_tasks == 0 && callback) {
        // Empty, call the callback in the next frame
        return pomelo_platform_submit_deferred_task(
            platform,
            NULL,
            callback,
            callback_data
        );
    }

    group->cancel_callback = callback;
    group->cancel_callback_data = callback_data;

    pomelo_platform_task_worker_t * worker_task = NULL;
    pomelo_list_for(
        group->worker_tasks,
        worker_task,
        pomelo_platform_task_worker_t *,
        {
            pomelo_platform_task_worker_cancel(
                worker_task,
                (pomelo_platform_task_cb)
                    pomelo_platform_task_group_on_worker_task_canceled,
                group
            );
        }
    );

    return 0;
}
