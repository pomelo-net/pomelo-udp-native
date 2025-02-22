#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pomelo-test.h"
#include "pomelo/address.h"
#include "base/payload.h"
#include "pomelo/allocator.h"
#include "utils/pool.h"
#include "utils/list.h"
#include "codec/codec.h"
#include "base-test.h"


int main(void) {
    printf("Base test\n");
    if (pomelo_codec_init() < 0) {
        printf("Failed to initialize codec \n");
        return -1;
    }

    pomelo_run_test(pomelo_test_address);
    pomelo_run_test(pomelo_test_payload);
    pomelo_run_test(pomelo_test_allocator);
    pomelo_run_test(pomelo_test_pool);
    pomelo_run_test(pomelo_test_list);
    pomelo_run_test(pomelo_test_unrolled_list);
    pomelo_run_test(pomelo_test_array);
    pomelo_run_test(pomelo_test_map);
    pomelo_run_test(pomelo_test_codec);
    pomelo_run_test(pomelo_test_reference);

    printf("*** All core tests passed ***\n");
    return 0;
}
