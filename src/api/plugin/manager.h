#ifndef POMELO_API_PLUGIN_MANAGER_SRC_H
#define POMELO_API_PLUGIN_MANAGER_SRC_H
#include "plugin.h"
#include "utils/list.h"
#include "utils/pool.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct pomelo_plugin_manager_options_s pomelo_plugin_manager_options_t;


/// @brief Creating options for plugin manager
struct pomelo_plugin_manager_options_s {
    /// @brief Allocator
    pomelo_allocator_t * allocator;
};


struct pomelo_plugin_manager_s {
    /// @brief The allocator
    pomelo_allocator_t * allocator;

    /// @brief List of plugins
    pomelo_list_t * plugins;

    /// @brief The template of plugin
    pomelo_plugin_t tpl;
};


/* -------------------------------------------------------------------------- */
/*                                Public APIs                                 */
/* -------------------------------------------------------------------------- */


/// @brief Initialize creating options of plugin manager
void pomelo_plugin_manager_options_init(
    pomelo_plugin_manager_options_t * options
);


/// @brief Create new plugin manager
pomelo_plugin_manager_t * pomelo_plugin_manager_create(
    pomelo_plugin_manager_options_t * options
);


/// @brief Destroy a plugin manager
void pomelo_plugin_manager_destroy(pomelo_plugin_manager_t * plugin_manager);


/// @brief Add a plugin
int pomelo_plugin_manager_add_plugin(
    pomelo_plugin_manager_t * plugin_manager,
    pomelo_plugin_t * plugin
);


#ifdef __cplusplus
}
#endif
#endif // ~POMELO_API_PLUGIN_MANAGER_SRC_H
