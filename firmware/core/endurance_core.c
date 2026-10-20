#include "endurance_core.h"

#include <stddef.h>

endurance_profile_status_t endurance_profile_validate(const endurance_profile_t *p)
{
    if (p == NULL) {
        return ENDURANCE_PROFILE_NO_CYCLES;
    }
    if (p->on_ms < ENDURANCE_MIN_PHASE_MS) {
        return ENDURANCE_PROFILE_ON_TOO_SHORT;
    }
    if (p->on_ms > ENDURANCE_MAX_PHASE_MS) {
        return ENDURANCE_PROFILE_ON_TOO_LONG;
    }
    if (p->off_ms < ENDURANCE_MIN_PHASE_MS) {
        return ENDURANCE_PROFILE_OFF_TOO_SHORT;
    }
    if (p->off_ms > ENDURANCE_MAX_PHASE_MS) {
        return ENDURANCE_PROFILE_OFF_TOO_LONG;
    }
    if (p->target_cycles < 1u) {
        return ENDURANCE_PROFILE_NO_CYCLES;
    }
    return ENDURANCE_PROFILE_OK;
}

void endurance_init(endurance_ctx_t *c)
{
    if (c == NULL) {
        return;
    }
    c->state = ENDURANCE_BOOT;
    c->profile.on_ms = 0u;
    c->profile.off_ms = 0u;
    c->profile.target_cycles = 0u;
    c->phase_start_ms = 0u;
    c->cycles_completed = 0u;
    c->fault = ENDURANCE_FAULT_NONE;
    c->fault_at_cycles_completed = 0u;
}

bool endurance_self_test_passed(endurance_ctx_t *c)
{
    if (c == NULL || c->state != ENDURANCE_BOOT) {
        return false;
    }
    c->state = ENDURANCE_IDLE;
    return true;
}

bool endurance_start(endurance_ctx_t *c, const endurance_profile_t *p, uint32_t now_ms)
{
    if (c == NULL || p == NULL) {
        return false;
    }
    if (c->state != ENDURANCE_IDLE && c->state != ENDURANCE_COMPLETE) {
        return false;
    }
    if (endurance_profile_validate(p) != ENDURANCE_PROFILE_OK) {
        return false;
    }
    c->profile = *p;
    c->cycles_completed = 0u;
    c->fault = ENDURANCE_FAULT_NONE;
    c->fault_at_cycles_completed = 0u;
    c->phase_start_ms = now_ms;
    c->state = ENDURANCE_RUNNING_ON;
    return true;
}

bool endurance_stop(endurance_ctx_t *c)
{
    if (c == NULL) {
        return false;
    }
    if (c->state != ENDURANCE_RUNNING_ON && c->state != ENDURANCE_RUNNING_OFF) {
        return false;
    }
    c->state = ENDURANCE_IDLE;
    return true;
}

void endurance_raise_fault(endurance_ctx_t *c, endurance_fault_t fault)
{
    if (c == NULL) {
        return;
    }
    if (c->state == ENDURANCE_FAULT) {
        return; /* keep the first fault as the root cause */
    }
    c->fault = (fault == ENDURANCE_FAULT_NONE) ? ENDURANCE_FAULT_INTERNAL : fault;
    c->fault_at_cycles_completed = c->cycles_completed;
    c->state = ENDURANCE_FAULT;
}

bool endurance_clear_fault(endurance_ctx_t *c, bool operator_authorized)
{
    if (c == NULL || c->state != ENDURANCE_FAULT || !operator_authorized) {
        return false;
    }
    c->state = ENDURANCE_IDLE;
    return true;
}

void endurance_tick(endurance_ctx_t *c, uint32_t now_ms)
{
    if (c == NULL) {
        return;
    }
    /* Unsigned subtraction keeps this correct across uint32_t wrap-around. */
    const uint32_t elapsed_ms = now_ms - c->phase_start_ms;

    if (c->state == ENDURANCE_RUNNING_ON) {
        if (elapsed_ms >= c->profile.on_ms) {
            c->state = ENDURANCE_RUNNING_OFF;
            c->phase_start_ms = now_ms;
        }
    } else if (c->state == ENDURANCE_RUNNING_OFF) {
        if (elapsed_ms >= c->profile.off_ms) {
            c->cycles_completed++; /* < target here, so cannot overflow */
            c->phase_start_ms = now_ms;
            c->state = (c->cycles_completed >= c->profile.target_cycles)
                           ? ENDURANCE_COMPLETE
                           : ENDURANCE_RUNNING_ON;
        }
    }
}

bool endurance_load_command(const endurance_ctx_t *c)
{
    return c != NULL && c->state == ENDURANCE_RUNNING_ON;
}
