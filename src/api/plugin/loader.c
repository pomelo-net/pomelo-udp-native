#include <assert.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <limits.h>
#endif

#include "pomelo/api.h"
#include "pomelo/plugin.h"


/// Name of register
#define POMELO_PLUGIN_REGISTER_ENTRY_NAME "pomelo_plugin_initializer_entry"


#ifdef _WIN32 // For Windows
static pomelo_plugin_initializer pomelo_plugin_load(const char * name) {
    HINSTANCE hinstance = LoadLibrary(name);
    if (!hinstance) {
        return NULL;
    }

    return (pomelo_plugin_initializer) GetProcAddress(
        hinstance,
        POMELO_PLUGIN_REGISTER_ENTRY_NAME
    );
}
#else

// For UNIX
static pomelo_plugin_initializer pomelo_plugin_load(const char * name) {
    assert(name != NULL);
    void * handle = dlopen(name, RTLD_LAZY);
    if (!handle) {
        return NULL;
    }
    dlerror(); // Clear error

    // Find the symbol
    void * fn = dlsym(handle, POMELO_PLUGIN_REGISTER_ENTRY_NAME);
    if (dlerror()) {
        dlclose(handle);
        return NULL;
    }

    return *((pomelo_plugin_initializer*) &fn);
}

#endif


pomelo_plugin_initializer pomelo_plugin_load_by_name(const char * name) {
    assert(name != NULL);
#ifdef _WIN32
    return pomelo_plugin_load(name); // Call directly
#else
    size_t length = strlen(name);
    char buffer[PATH_MAX];
#ifdef __APPLE__ // MacOS -> Prefix `lib` and postfix `.dylib` -> 9 characters
    if (length > (PATH_MAX - 10)) { // Invalid library name
        return NULL;
    }
    snprintf(buffer, sizeof(buffer), "lib%s.dylib", name);
#else // Linux -> Prefix `lib` and postfix `.so` -> 6 characters
    if (length > (PATH_MAX - 7)) { // Invalid library name
        return NULL;
    }
    snprintf(buffer, sizeof(buffer), "lib%s.so");
#endif
    return pomelo_plugin_load(buffer);
#endif
}


pomelo_plugin_initializer pomelo_plugin_load_by_path(const char * path) {
    assert(path != NULL);
    return pomelo_plugin_load(path);
}
