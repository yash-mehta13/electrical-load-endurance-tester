#include "debounce.h"

#include <stddef.h>

void debounce_reset(debounce_t *d)
{
    if (d != NULL) {
        d->consecutive = 0u;
    }
}

bool debounce_update(debounce_t *d, bool condition_true, uint32_t required)
{
    if (d == NULL) {
        return false;
    }
    if (required == 0u) {
        required = 1u;
    }
    if (!condition_true) {
        d->consecutive = 0u;
        return false;
    }
    if (d->consecutive < required) {
        d->consecutive++;
    }
    return d->consecutive >= required;
}
