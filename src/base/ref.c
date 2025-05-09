#include <assert.h>
#include <stddef.h>
#include "ref.h"


void pomelo_reference_init(
    pomelo_reference_t * object,
    pomelo_ref_finalize_cb finalize_cb
) {
    assert(object != NULL);
    pomelo_atomic_int64_store(&object->ref_counter, 1);
    object->finalize_cb = finalize_cb;
}


bool pomelo_reference_ref(pomelo_reference_t * object) {
    assert(object != NULL);
    int64_t ref, next;
    bool result;
    do {
        ref = pomelo_atomic_int64_load(&object->ref_counter);
        if (ref <= 0) {
            assert(false && "Try to ref finalized reference");
            return false;
        }
        next = ref + 1;
        result = pomelo_atomic_int64_compare_exchange(
            &object->ref_counter,
            ref,
            next
        );
    } while (!result);

    return true;
}


void pomelo_reference_unref(pomelo_reference_t * object) {
    assert(object != NULL);

    int64_t ref, next;
    bool result;
    do {
        ref = pomelo_atomic_int64_load(&object->ref_counter);
        if (ref <= 0) {
            assert(false && "Try to unref finalized reference");
            return;
        }
        next = ref - 1;
        result = pomelo_atomic_int64_compare_exchange(
            &object->ref_counter,
            ref,
            next
        );
    } while (!result);

    if (next == 0) {
        if (object->finalize_cb) {
            // Finalize this ref
            object->finalize_cb(object);
        }
    }
}
