#include <assert.h>
#include <string.h>
#include "args.h"

#define str_empty(str) ((str)[0] == '\0')

#define str_equals(str_1, str_2)                       \
    ((str_1) && (str_2) && strcmp(str_1, str_2) == 0)

#define descriptor_has_arg(descriptor, arg) (          \
    str_equals(arg, descriptor.arg_short) ||           \
    str_equals(arg, descriptor.arg_long)               \
)

void pomelo_arg_process(
    int argc,
    char * argv[],
    pomelo_arg_descriptor_t * descriptors,
    pomelo_arg_vector_t * vectors,
    int ndescriptors
) {
    assert(argv != NULL);
    assert(descriptors != NULL);
    assert(vectors != NULL);
    
    // Current checking vector
    pomelo_arg_vector_t * vector = NULL;

    // Ignore the first argument
    for (int i = 0; i < argc; i++) {
        char * arg = argv[i];
        if (str_empty(arg)) {
            continue; // Ignore empty arg
        }

        bool is_arg_type = false;
        for (int j = 0; j < ndescriptors; j++) {
            if (descriptor_has_arg(descriptors[j], arg)) {
                vector = &vectors[j];
                vector->present = true;
                vector->begin = 0;
                vector->end = 0;
                is_arg_type = true;
                break;
            }
        }

        if (is_arg_type || !vector) {
            continue; // Ingore
        }

        if (vector->begin == 0) {
            vector->begin = i;
        }
        vector->end = i;
    }
}
