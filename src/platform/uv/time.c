#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif
#include "uv.h"
#include "platform/platform.h"

uint64_t pomelo_platform_hrtime(pomelo_platform_t * platform) {
    (void) (platform);
    return uv_hrtime();
}

// The API uv_clock_gettime is only avaiable in version 1.45.0 of libuv

#if UV_VERSION_HEX >= ((1 << 16) | (45 << 8) | 0)

uint64_t pomelo_platform_now(pomelo_platform_t * platform) {
    (void) platform;
    uv_timespec64_t timespec;

    if (uv_clock_gettime(UV_CLOCK_REALTIME, &timespec) < 0) {
        return 0;
    }

    return timespec.tv_sec * 1000ULL + timespec.tv_nsec / 1000000ULL;
}

#else

#ifdef _WIN32

uint64_t pomelo_platform_now(pomelo_platform_t * platform) {
    // Win32 system
    (void) platform;

    FILETIME ft;
    int64_t t;

    GetSystemTimePreciseAsFileTime(&ft);
    /* In 100-nanosecond increments from 1601-01-01 UTC because why not? */
    t = (int64_t) ft.dwHighDateTime << 32 | ft.dwLowDateTime;
    /* Convert to UNIX epoch, 1970-01-01. Still in 100 ns increments. */
    t -= 116444736000000000ll;
    /* Now convert to seconds and nanoseconds. */

    return t / 10000;
}

#else

uint64_t pomelo_platform_now(pomelo_platform_t * platform) {
    // Unix system
    (void) platform;

    struct timespec t;
    int r = clock_gettime(CLOCK_REALTIME, &t);

    if (r) {
        return 0;
    }

    return t.tv_sec * 1000ULL + t.tv_nsec / 1000000ULL;
}

#endif // _WIN32
#endif // UV_VERSION_HEX
