#include <string.h>
#include "pomelo-test.h"
#include "base/payload.h"
#include "base-test.h"


int pomelo_test_payload(void) {
    uint8_t data[16];

    pomelo_payload_t payload;
    payload.capacity = 16;
    payload.data = data;
    payload.position = 0;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;

    memset(data, 0, sizeof(data));

    pomelo_check(pomelo_payload_write_uint8(&payload, 12) == 0);
    pomelo_check(pomelo_payload_write_uint16(&payload, 450) == 0);
    pomelo_check(pomelo_payload_write_uint32(&payload, 1051411350U) == 0);
    pomelo_check(pomelo_payload_write_uint64(&payload, 121121121233ULL) == 0);
    
    // overflow
    pomelo_check(pomelo_payload_write_uint64(&payload, 192987) != 0);
    
    pomelo_check(payload.position == 15);

    // Reset position
    payload.position = 0;

    pomelo_check(pomelo_payload_read_uint8(&payload, &u8) == 0);

    pomelo_check(u8 == 12);
    pomelo_check(pomelo_payload_read_uint16(&payload, &u16) == 0);
    pomelo_check(u16 == 450);
    pomelo_check(pomelo_payload_read_uint32(&payload, &u32) == 0);
    pomelo_check(u32 == 1051411350U);
    pomelo_check(pomelo_payload_read_uint64(&payload, &u64) == 0);
    pomelo_check(u64 == 121121121233ULL);
    
    // under flow
    pomelo_check(pomelo_payload_read_uint64(&payload, &u64) != 0);

    pomelo_check(payload.position == 15);

    payload.position = 0;
    pomelo_check(pomelo_payload_write_int8(&payload, 112) == 0);
    pomelo_check(pomelo_payload_write_int16(&payload, -450) == 0);
    pomelo_check(pomelo_payload_write_int32(&payload, 1211211233) == 0);
    pomelo_check(pomelo_payload_write_int64(&payload, -121121121233LL) == 0);
    
    // overflow
    pomelo_check(pomelo_payload_write_int64(&payload, 192987) != 0);
    
    pomelo_check(payload.position == 15);

    // Reset position
    payload.position = 0;

    pomelo_check(pomelo_payload_read_int8(&payload, &i8) == 0);

    pomelo_check(i8 == 112);
    pomelo_check(pomelo_payload_read_int16(&payload, &i16) == 0);
    pomelo_check(i16 == -450);
    pomelo_check(pomelo_payload_read_int32(&payload, &i32) == 0);
    pomelo_check(i32 == 1211211233);
    pomelo_check(pomelo_payload_read_int64(&payload, &i64) == 0);
    pomelo_check(i64 == -121121121233LL);

    pomelo_check(payload.position == 15);

    payload.position = 0;
    pomelo_check(pomelo_payload_write_float32(&payload, -120.2f) == 0);
    pomelo_check(pomelo_payload_write_float64(&payload, 10002.212) == 0);
    pomelo_check(payload.position == 12);

    payload.position = 0;
    pomelo_check(pomelo_payload_read_float32(&payload, &f32) == 0);
    pomelo_check(f32 == -120.2f);    
    pomelo_check(pomelo_payload_read_float64(&payload, &f64) == 0);
    pomelo_check(f64 == 10002.212);
    pomelo_check(payload.position == 12);

    return 0;
}
