#include <assert.h>
#include <string.h>
#include "manager.h"
#include "plugin.h"


pomelo_plugin_manager_t * pomelo_plugin_manager_create(
    pomelo_plugin_manager_options_t * options
) {
    assert(options != NULL);
    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
        if (!allocator) {
            return NULL;
        }
    }

    pomelo_plugin_manager_t * plugin_manager =
        pomelo_allocator_malloc_t(allocator, pomelo_plugin_manager_t);
    if (!plugin_manager) {
        // Failed to allocate plugin manager
        return NULL;
    }

    memset(plugin_manager, 0, sizeof(pomelo_plugin_manager_t));
    plugin_manager->allocator = allocator;

    // Create list of plugins
    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_plugin_t *)
    };
    pomelo_list_t * plugins = pomelo_list_create(&list_options);
    if (!plugins) {
        pomelo_plugin_manager_destroy(plugin_manager);
        return NULL;
    }

    plugin_manager->plugins = plugins;

    // Initialize plugin template
    pomelo_plugin_init_template(&plugin_manager->tpl);

    return plugin_manager;
}


void pomelo_plugin_manager_destroy(pomelo_plugin_manager_t * plugin_manager) {
    assert(plugin_manager != NULL);

    // Destroy all plugins
    pomelo_plugin_t * plugin = NULL;
    while (pomelo_list_pop_front(plugin_manager->plugins, &plugin) == 0) {
        pomelo_plugin_destroy((pomelo_plugin_impl_t *) plugin);
    }

    // Destroy list of plugins
    if (plugin_manager->plugins) {
        pomelo_list_destroy(plugin_manager->plugins);
        plugin_manager->plugins = NULL;
    }

    // Destroy the manager itself
    pomelo_allocator_free(plugin_manager->allocator, plugin_manager);
}


int pomelo_plugin_manager_add_plugin(
    pomelo_plugin_manager_t * plugin_manager,
    pomelo_plugin_t * plugin
) {
    assert(plugin_manager != NULL);
    assert(plugin != NULL);

    // Update the template
    *plugin = plugin_manager->tpl;

    return pomelo_list_push_back(plugin_manager->plugins, plugin) ? 0 : -1;
}


/* -------------------------------------------------------------------------- */
/*                               Private APIs                                 */
/* -------------------------------------------------------------------------- */
