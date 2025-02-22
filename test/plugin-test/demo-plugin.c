#include <stdio.h>
#include "pomelo/plugin.h"

// Just for testing loader
void demo_plugin_entry(pomelo_plugin_t * plugin) {
    // Just print out
    printf("Demo plugin initializer has been called\n");
}
POMELO_PLUGIN_ENTRY_REGISTER(demo_plugin_entry);
