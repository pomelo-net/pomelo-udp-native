#include "uv.h"


static uv_work_t work_req;


void work_callback(uv_work_t * req) {
    (void) req;
    printf("Doing work in thread pool!\n");
    uv_sleep(1000);
    printf("Work done!\n");
}


void after_work_callback(uv_work_t * req, int status) {
    (void) req;
    printf("Work complete! %d\n", status);
}


void timer_callback(uv_timer_t * timer) {
    (void) timer;
    printf("Timer callback!\n");
    int ret = uv_cancel((uv_req_t *) &work_req);
    printf("Cancel result: %d\n", ret);
}

int main(void) {
    uv_loop_t loop;
    uv_loop_init(&loop);

    // Work request
    work_req.data = NULL;

    // Queue work on thread pool
    uv_queue_work(&loop, &work_req, work_callback, after_work_callback);
    timer_callback(NULL);

    // Timer
    uv_timer_t timer;
    uv_timer_init(&loop, &timer);
    uv_timer_start(&timer, timer_callback, 500, 0);

    // Run event loop
    uv_run(&loop, UV_RUN_DEFAULT);

    // Cleanup
    uv_loop_close(&loop);

    return 0;
}
