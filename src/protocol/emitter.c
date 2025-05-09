#include <assert.h>
#include <string.h>
#include "utils/macro.h"
#include "sender.h"
#include "emitter.h"
#include "context.h"
#include "client.h"

/* -------------------------------------------------------------------------- */
/*                            Frequent emitter                                */
/* -------------------------------------------------------------------------- */


int pomelo_protocol_emitter_init(
    pomelo_protocol_emitter_t * emitter,
    pomelo_protocol_emitter_options_t * options
) {
    assert(emitter != NULL);
    assert(options != NULL);

    memset(emitter, 0, sizeof(pomelo_protocol_emitter_t));
    emitter->client = options->client;
    emitter->frequency = options->frequency;
    emitter->limit = options->limit;
    emitter->timeout_ms = options->timeout_ms;
    emitter->trigger_cb = options->trigger_cb;
    emitter->timeout_cb = options->timeout_cb;
    emitter->limit_cb = options->limit_cb;

    // Initialize the trigger task
    pomelo_sequencer_task_init(
        &emitter->trigger_task,
        (pomelo_sequencer_callback) pomelo_protocol_emitter_on_triggered,
        emitter
    );

    // Initialize the timeout task
    pomelo_sequencer_task_init(
        &emitter->timeout_task,
        (pomelo_sequencer_callback) pomelo_protocol_emitter_on_timeout,
        emitter
    );

    return 0;
}


static void emitter_on_triggered(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);
    pomelo_sequencer_submit(
        emitter->client->socket.sequencer,
        &emitter->trigger_task
    );
}


static void emitter_on_timeout(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);
    pomelo_sequencer_submit(
        emitter->client->socket.sequencer,
        &emitter->timeout_task
    );
}


int pomelo_protocol_emitter_start(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);
    if (emitter->running) {
        return -1; // The emitter has already been running
    }

    // Check parameters
    if (emitter->frequency == 0) {
        return -1; // Invalid frequency
    }

    pomelo_protocol_client_t * client = emitter->client;
    pomelo_platform_t * platform = client->socket.platform;

    int ret = pomelo_platform_timer_start(
        platform,
        (pomelo_platform_timer_entry) emitter_on_triggered,
        0, // No timeout
        POMELO_FREQ_TO_MS(emitter->frequency),
        emitter,
        &emitter->trigger_timer
    );

    if (ret < 0) {
        pomelo_protocol_emitter_stop(emitter);
        return -1; // Failed to start timer
    }
    
    // Check timer
    if (emitter->timeout_ms > 0 && emitter->timeout_cb) {
        ret = pomelo_platform_timer_start(
            platform,
            (pomelo_platform_timer_entry) emitter_on_timeout,
            emitter->timeout_ms,
            emitter->timeout_ms, // With repeat
            emitter,
            &emitter->timeout_timer
        );

        if (ret < 0) {
            pomelo_protocol_emitter_stop(emitter);
            return -1; // Failed to start timer
        }
    }

    emitter->trigger_counter = 0;
    emitter->running = true;
    return 0;
}


void pomelo_protocol_emitter_stop(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);

    if (!emitter->running) {
        return; // The emitter has already been stopped
    }

    // Just stop & destroy the timer
    pomelo_platform_t * platform = emitter->client->socket.platform;
    pomelo_platform_timer_stop(platform, &emitter->trigger_timer);
    pomelo_platform_timer_stop(platform, &emitter->timeout_timer);
    emitter->running = false;
}


void pomelo_protocol_emitter_on_triggered(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);

    if (emitter->trigger_cb) {
        emitter->trigger_cb(emitter->client);
    }

    emitter->trigger_counter++;
    if (emitter->limit > 0 && emitter->limit == emitter->trigger_counter) {
        // Reach the limit, stop emitting
        pomelo_protocol_emitter_stop(emitter);

        // Then call the callback
        if (emitter->limit_cb) {
            emitter->limit_cb(emitter->client);
        }
    }
}


void pomelo_protocol_emitter_on_timeout(pomelo_protocol_emitter_t * emitter) {
    assert(emitter != NULL);
    emitter->timeout_cb(emitter->client);
}
