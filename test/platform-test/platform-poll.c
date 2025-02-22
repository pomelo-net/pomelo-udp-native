#include "uv.h"
#include "platform-test.h"
#include "pomelo/platforms/platform-poll.h"


#define FRAME_TIME_MS 16

pomelo_platform_t * pomelo_test_platform_create(
    pomelo_allocator_t * allocator
) {
    pomelo_platform_poll_options_t options;
    pomelo_platform_poll_options_init(&options);
    options.allocator = allocator;
    return pomelo_platform_poll_create(&options);
}


void pomelo_test_platform_destroy(pomelo_platform_t * platform) {
    pomelo_platform_poll_destroy(platform);
}


void pomelo_test_platform_run(pomelo_platform_t * platform) {
    while (pomelo_platform_poll_service(platform) > 0) {
        uv_sleep(FRAME_TIME_MS);
    }
}
