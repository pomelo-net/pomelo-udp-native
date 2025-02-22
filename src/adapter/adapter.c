#include <assert.h>
#include <string.h>
#include "adapter.h"


void pomelo_adapter_options_init(pomelo_adapter_options_t * options) {
    assert(options != NULL);
    memset(options, 0, sizeof(pomelo_adapter_options_t));
}
