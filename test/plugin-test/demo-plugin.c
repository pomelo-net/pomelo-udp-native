#include <stdio.h>
#include "pomelo/plugin.h"


static int extra_data = 1234;


// Just for testing loader
void demo_plugin_entry(pomelo_plugin_t * plugin) {
    // Just print out
    printf("Demo plugin initializer has been called\n");
    plugin->set_data(plugin, &extra_data);
}
POMELO_PLUGIN_ENTRY_REGISTER(demo_plugin_entry);
