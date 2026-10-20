/*
 * Endurance-cycle scheduler core (hardware-independent).
 *
 * STATUS: host-unit-tested only (tests/unit). NOT integrated into any
 * ESP-IDF build and NOT verified against real switching hardware.
 *
 * Design rules:
 *  - No ESP-IDF, FreeRTOS, heap, or I/O dependencies; all time and events
 *    are passed in by the caller so behaviour is deterministic and testable.
 *  - The load command is a pure function of the state: it is true only in
 *    ENDURANCE_RUNNING_ON. Every other state commands the load OFF.
 *  - Time is a free-running uint32_t millisecond counter. All elapsed-time
 *    comparisons use unsigned subtraction, so counter wrap-around is safe as
 *    long as endurance_tick() runs more often than every 2^31 ms and phase
 *    durations stay below ENDURANCE_MAX_PHASE_MS.
 *  - A cycle is "completed" only when its ON interval and its following OFF
 *    interval have both fully elapsed. Aborted or faulted cycles are never
 *    counted.
 */
#ifndef ENDURANCE_CORE_H
#define ENDURANCE_CORE_H

#include <stdbool.h>
#include <stdint.h>

/* Provisional software limits. They are NOT derived from any load rating or
 * hardware limit; tighten them once the switching stage is chosen. */
#define ENDURANCE_MIN_PHASE_MS 10u          /* >= a few scheduler ticks */
#define ENDURANCE_MAX_PHASE_MS 86400000u    /* 24 h; must stay < 2^31 ms */

typedef enum {
    ENDURANCE_BOOT = 0,   /* load OFF; waiting for self-test result        */
    ENDURANCE_IDLE,       /* self-test passed; test may be started         */
    ENDURANCE_RUNNING_ON, /* only state that commands the load ON          */
    ENDURANCE_RUNNING_OFF,
    ENDURANCE_COMPLETE,   /* target reached; load OFF                      */
    ENDURANCE_FAULT       /* latched; load OFF; manual clear required      */
} endurance_state_t;

/* Provisional enumeration; numeric values must not be exposed on CAN/UART
 * until the fault policy is finalised (see docs/fault_policy.md). */
typedef enum {
    ENDURANCE_FAULT_NONE = 0,
    ENDURANCE_FAULT_OVERCURRENT,
    ENDURANCE_FAULT_SHORT_CIRCUIT,
    ENDURANCE_FAULT_OPEN_LOAD,
    ENDURANCE_FAULT_OVERTEMPERATURE,
    ENDURANCE_FAULT_UNDERVOLTAGE,
    ENDURANCE_FAULT_SENSOR_INVALID,
    ENDURANCE_FAULT_INTERNAL
} endurance_fault_t;

typedef enum {
    ENDURANCE_PROFILE_OK = 0,
    ENDURANCE_PROFILE_ON_TOO_SHORT,
    ENDURANCE_PROFILE_ON_TOO_LONG,
    ENDURANCE_PROFILE_OFF_TOO_SHORT,
    ENDURANCE_PROFILE_OFF_TOO_LONG,
    ENDURANCE_PROFILE_NO_CYCLES
} endurance_profile_status_t;

typedef struct {
    uint32_t on_ms;
    uint32_t off_ms;
    uint32_t target_cycles; /* must be >= 1; there is no "run forever" value */
} endurance_profile_t;

typedef struct {
    endurance_state_t state;
    endurance_profile_t profile;
    uint32_t phase_start_ms;
    uint32_t cycles_completed;
    endurance_fault_t fault;
    uint32_t fault_at_cycles_completed; /* cycles completed when fault latched */
} endurance_ctx_t;

endurance_profile_status_t endurance_profile_validate(const endurance_profile_t *p);

/* Puts the context in BOOT with the load commanded OFF. */
void endurance_init(endurance_ctx_t *c);

/* Caller reports a passed self-test: BOOT -> IDLE. Returns false otherwise. */
bool endurance_self_test_passed(endurance_ctx_t *c);

/* IDLE or COMPLETE -> RUNNING_ON with counters reset. Rejected (false) from
 * any other state, including FAULT, or for an invalid profile. */
bool endurance_start(endurance_ctx_t *c, const endurance_profile_t *p, uint32_t now_ms);

/* RUNNING_* -> IDLE, load OFF at once; the partial cycle is not counted.
 * Returns false (no-op) in any other state. */
bool endurance_stop(endurance_ctx_t *c);

/* Latches a fault from ANY state: load OFF, state FAULT. If already in FAULT
 * the first fault code is kept. Passing ENDURANCE_FAULT_NONE is mapped to
 * ENDURANCE_FAULT_INTERNAL so a fault can never be latched without a code. */
void endurance_raise_fault(endurance_ctx_t *c, endurance_fault_t fault);

/* FAULT -> IDLE, only when `operator_authorized` is true. Does not resume the
 * interrupted test. Returns false otherwise. */
bool endurance_clear_fault(endurance_ctx_t *c, bool operator_authorized);

/* Advances the schedule. At most one phase transition per call. A late call
 * never replays missed phases and never shortens the next phase: the next
 * phase is timed from the moment the transition is processed. */
void endurance_tick(endurance_ctx_t *c, uint32_t now_ms);

/* true only in ENDURANCE_RUNNING_ON. */
bool endurance_load_command(const endurance_ctx_t *c);

#endif /* ENDURANCE_CORE_H */
