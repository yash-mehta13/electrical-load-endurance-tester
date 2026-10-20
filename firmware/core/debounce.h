/*
 * Consecutive-sample qualifier (hardware-independent). Host-unit-tested only.
 *
 * A condition trips only after it has been observed true for `required`
 * consecutive samples; any false sample restarts the count. The tripped
 * state is not latched here: it stays true while the condition stays true.
 * Latching and recovery belong to the fault manager, not this helper.
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t consecutive;
} debounce_t;

void debounce_reset(debounce_t *d);

/* required == 0 is treated as 1 (trip on the first true sample).
 * The counter saturates at `required`, so it cannot overflow. */
bool debounce_update(debounce_t *d, bool condition_true, uint32_t required);

#endif /* DEBOUNCE_H */
