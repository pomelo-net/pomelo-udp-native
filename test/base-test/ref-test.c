#include <stdbool.h>
#include "pomelo-test.h"
#include "base/ref.h"
#include "base-test.h"


static bool finalized = false;


static void reference_finalize(pomelo_reference_t * reference) {
    (void) reference;
    finalized = true;
}


int pomelo_test_reference(void) {
    pomelo_reference_t ref;
    pomelo_reference_init(&ref, reference_finalize);

    pomelo_check(pomelo_reference_ref_count(&ref) == 1);
    pomelo_check(pomelo_reference_ref(&ref) == true);
    pomelo_check(pomelo_reference_ref_count(&ref) == 2);
    pomelo_check(finalized == false);
    pomelo_reference_unref(&ref);
    pomelo_check(pomelo_reference_ref_count(&ref) == 1);
    pomelo_check(finalized == false);
    pomelo_reference_unref(&ref);
    pomelo_check(finalized == true);
    pomelo_check(pomelo_reference_ref_count(&ref) == 0);

    return 0;
}
