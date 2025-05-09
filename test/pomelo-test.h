#ifndef POMELO_TEST_H
#define POMELO_TEST_H
#include <stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


/// This will help the pomelo_test_main return negative error on failure
#define pomelo_run_test(func)                                                  \
do {                                                                           \
    printf("[+] Run " #func "...\n");                                          \
    int ret = func();                                                          \
    if (ret != 0) {                                                            \
        printf("=> Failed\n\n");                                               \
        abort();                                                               \
    }                                                                          \
    printf("=> OK\n\n");                                                       \
} while(0)


#define pomelo_check(condition)                                                \
do {                                                                           \
    if (!(condition)) {                                                        \
        printf(                                                                \
            "[!] Failed at:\n    '%s' in '%s' %s:%d.\n",                       \
            #condition,                                                        \
            __func__,                                                          \
            __FILE__,                                                          \
            __LINE__                                                           \
        );                                                                     \
        abort();                                                               \
    }                                                                          \
} while(0)


#define pomelo_track_function() printf("[i] %s\n", __func__)


#ifdef __cplusplus
}
#endif
#endif // POMELO_TEST_H
