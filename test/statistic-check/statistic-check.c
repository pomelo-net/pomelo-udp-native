#include <assert.h>
#include <string.h>
#include "statistic-check.h"
#include "pomelo-test.h"


void pomelo_statistic_check_resource_leak(pomelo_statistic_t * statistic) {
    assert(statistic != NULL);
    pomelo_statistic_api_check_resource_leak(&statistic->api);
    pomelo_statistic_delivery_check_resource_leak(&statistic->delivery);
    pomelo_statistic_protocol_check_resource_leak(&statistic->protocol);
    pomelo_statistic_buffer_check_resource_leak(&statistic->buffer);
}


void pomelo_statistic_protocol_check_resource_leak(
    pomelo_statistic_protocol_t * protocol
) {
    assert(protocol != NULL);

    pomelo_check(protocol->senders == 0);
    pomelo_check(protocol->receivers == 0);
    pomelo_check(protocol->packets == 0);
    pomelo_check(protocol->peers == 0);
    pomelo_check(protocol->servers == 0);
    pomelo_check(protocol->clients == 0);
    pomelo_check(protocol->crypto_contexts == 0);
    pomelo_check(protocol->acceptances == 0);
}


void pomelo_statistic_delivery_check_resource_leak(
    pomelo_statistic_delivery_t * delivery
) {
    assert(delivery != NULL);

    pomelo_check(delivery->dispatchers == 0);
    pomelo_check(delivery->senders == 0);
    pomelo_check(delivery->receivers == 0);
    pomelo_check(delivery->endpoints == 0);
    pomelo_check(delivery->buses == 0);
    pomelo_check(delivery->receptions == 0);
    pomelo_check(delivery->transmissions == 0);
    pomelo_check(delivery->parcels == 0);
}


void pomelo_statistic_api_check_resource_leak(pomelo_statistic_api_t * api) {
    assert(api != NULL);

    assert(api->messages == 0);
    pomelo_check(api->builtin_sessions == 0);
    pomelo_check(api->plugin_sessions == 0);
    pomelo_check(api->builtin_channels == 0);
    pomelo_check(api->plugin_channels == 0);
}


void pomelo_statistic_buffer_check_resource_leak(
    pomelo_statistic_buffer_t * buffer
) {
    assert(buffer != NULL);
    pomelo_check(buffer->buffers == 0);
}
