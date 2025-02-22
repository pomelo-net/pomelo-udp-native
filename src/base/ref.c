#include <assert.h>
#include <stddef.h>
#include "ref.h"

#ifdef POMELO_MULTI_THREAD

#define pomelo_ref_counter_increase(ref_counter)                               \
    pomelo_atomic_int64_fetch_add(&(ref_counter), 1) + 1
#define pomelo_ref_counter_decrease(ref_counter)                               \
    pomelo_atomic_int64_fetch_sub(&(ref_counter), 1) - 1
#define pomelo_ref_counter_set(ref_counter, value)                             \
    pomelo_atomic_int64_store(&(ref_counter), value)
#define pomelo_ref_counter_compare_exchange(ref_counter, expected, value)      \
    pomelo_atomic_int64_compare_exchange(&(ref_counter), expected, value)

#else // !POMELO_MULTI_THREAD

#define pomelo_ref_counter_increase(ref_counter) (++(ref_counter))
#define pomelo_ref_counter_decrease(ref_counter) (--(ref_counter))
#define pomelo_ref_counter_set(ref_counter, value) (ref_counter) = value

#endif // POMELO_MULTI_THREAD


void pomelo_reference_init(
    pomelo_reference_t * object,
    pomelo_ref_finalize_cb finalize_cb
) {
    assert(object != NULL);
    pomelo_ref_counter_set(object->ref_counter, 1);
    object->finalize_cb = finalize_cb;
}


bool pomelo_reference_ref(pomelo_reference_t * object) {
    assert(object != NULL);
#ifdef POMELO_MULTI_THREAD
    int64_t ref, next;
    do {
        ref = pomelo_ref_counter_get(object->ref_counter);
        if (ref <= 0) {
            assert(false && "Try to ref finalized reference");
            return false;
        }
        next = ref + 1;
    } while (
        !pomelo_ref_counter_compare_exchange(object->ref_counter, ref, next)
    );
#else // !POMELO_MULTI_THREAD
    int64_t ref = pomelo_ref_counter_get(object->ref_counter);
    if (ref <= 0) {
        assert(false && "Try to ref finalized reference");
        return false;
    }
    pomelo_ref_counter_increase(object->ref_counter);
#endif // POMELO_MULTI_THREAD
    return true;
}


void pomelo_reference_unref(pomelo_reference_t * object) {
    assert(object != NULL);
    int64_t ref = pomelo_ref_counter_decrease(object->ref_counter);
    assert(ref >= 0);

    if (ref == 0) {
        if (object->finalize_cb) {
            // Finalize this ref
            object->finalize_cb(object);
        }
    }
}
