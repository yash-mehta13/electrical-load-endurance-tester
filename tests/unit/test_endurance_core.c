/*
 * Host unit tests for firmware/core. Plain C, no framework.
 * These are software tests of decision logic only. They say nothing about
 * physical switching behaviour, sensor accuracy, or fault reaction time.
 */
#include <inttypes.h>
#include <stdio.h>

#include "../../firmware/core/debounce.h"
#include "../../firmware/core/endurance_core.h"

static int g_checks;
static int g_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            g_failures++;                                                    \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
        }                                                                    \
    } while (0)

static const endurance_profile_t P = {.on_ms = 100u, .off_ms = 200u, .target_cycles = 2u};

static endurance_ctx_t idle_ctx(void)
{
    endurance_ctx_t c;
    endurance_init(&c);
    endurance_self_test_passed(&c);
    return c;
}

static void test_profile_validation(void)
{
    endurance_profile_t p = P;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_OK);
    p.on_ms = ENDURANCE_MIN_PHASE_MS - 1u;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_ON_TOO_SHORT);
    p.on_ms = ENDURANCE_MIN_PHASE_MS;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_OK);
    p.on_ms = ENDURANCE_MAX_PHASE_MS + 1u;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_ON_TOO_LONG);
    p = P;
    p.off_ms = 0u;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_OFF_TOO_SHORT);
    p.off_ms = ENDURANCE_MAX_PHASE_MS;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_OK);
    p.off_ms = ENDURANCE_MAX_PHASE_MS + 1u;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_OFF_TOO_LONG);
    p = P;
    p.target_cycles = 0u;
    CHECK(endurance_profile_validate(&p) == ENDURANCE_PROFILE_NO_CYCLES);
    CHECK(endurance_profile_validate(NULL) != ENDURANCE_PROFILE_OK);
    CHECK(ENDURANCE_MAX_PHASE_MS < 0x80000000u);
}

static void test_boot_gate(void)
{
    endurance_ctx_t c;
    endurance_init(&c);
    CHECK(c.state == ENDURANCE_BOOT);
    CHECK(!endurance_load_command(&c));
    CHECK(!endurance_start(&c, &P, 0u)); /* no start before self-test */
    CHECK(c.state == ENDURANCE_BOOT);
    CHECK(endurance_self_test_passed(&c));
    CHECK(c.state == ENDURANCE_IDLE);
    CHECK(!endurance_self_test_passed(&c));
}

static void test_start_rejects_invalid_profile(void)
{
    endurance_ctx_t c = idle_ctx();
    endurance_profile_t bad = P;
    bad.target_cycles = 0u;
    CHECK(!endurance_start(&c, &bad, 0u));
    CHECK(c.state == ENDURANCE_IDLE);
    CHECK(!endurance_start(&c, NULL, 0u));
    CHECK(!endurance_load_command(&c));
}

static void test_nominal_schedule(void)
{
    endurance_ctx_t c = idle_ctx();
    CHECK(endurance_start(&c, &P, 1000u));
    CHECK(c.state == ENDURANCE_RUNNING_ON);
    CHECK(endurance_load_command(&c));

    endurance_tick(&c, 1099u); /* one ms before the ON interval ends */
    CHECK(c.state == ENDURANCE_RUNNING_ON);
    endurance_tick(&c, 1100u);
    CHECK(c.state == ENDURANCE_RUNNING_OFF);
    CHECK(!endurance_load_command(&c));
    CHECK(c.cycles_completed == 0u); /* ON done, cycle not yet complete */

    endurance_tick(&c, 1299u);
    CHECK(c.state == ENDURANCE_RUNNING_OFF);
    endurance_tick(&c, 1300u);
    CHECK(c.state == ENDURANCE_RUNNING_ON);
    CHECK(c.cycles_completed == 1u);

    endurance_tick(&c, 1400u);
    CHECK(c.state == ENDURANCE_RUNNING_OFF);
    CHECK(c.cycles_completed == 1u);
    endurance_tick(&c, 1600u);
    CHECK(c.state == ENDURANCE_COMPLETE);
    CHECK(c.cycles_completed == 2u);
    CHECK(!endurance_load_command(&c));

    endurance_tick(&c, 99999u); /* no change once complete */
    CHECK(c.state == ENDURANCE_COMPLETE);
    CHECK(c.cycles_completed == 2u);

    CHECK(endurance_start(&c, &P, 5000u)); /* restart from COMPLETE resets counters */
    CHECK(c.cycles_completed == 0u);
}

static void test_stop_does_not_count_partial_cycle(void)
{
    endurance_ctx_t c = idle_ctx();
    endurance_start(&c, &P, 0u);
    endurance_tick(&c, 100u);
    CHECK(c.state == ENDURANCE_RUNNING_OFF);
    CHECK(endurance_stop(&c)); /* stop during OFF of cycle 1 */
    CHECK(c.state == ENDURANCE_IDLE);
    CHECK(c.cycles_completed == 0u);
    CHECK(!endurance_load_command(&c));
    CHECK(!endurance_stop(&c)); /* second stop is a no-op */

    endurance_start(&c, &P, 0u);
    CHECK(endurance_stop(&c)); /* stop during ON */
    CHECK(!endurance_load_command(&c));
    CHECK(c.cycles_completed == 0u);
}

static void test_fault_latches_and_blocks_restart(void)
{
    endurance_ctx_t c = idle_ctx();
    endurance_start(&c, &P, 0u);
    endurance_tick(&c, 100u);
    endurance_tick(&c, 300u); /* cycle 1 done, now ON */
    CHECK(c.cycles_completed == 1u && endurance_load_command(&c));

    endurance_raise_fault(&c, ENDURANCE_FAULT_OVERCURRENT);
    CHECK(c.state == ENDURANCE_FAULT);
    CHECK(!endurance_load_command(&c));
    CHECK(c.fault == ENDURANCE_FAULT_OVERCURRENT);
    CHECK(c.fault_at_cycles_completed == 1u);

    endurance_raise_fault(&c, ENDURANCE_FAULT_UNDERVOLTAGE); /* first fault kept */
    CHECK(c.fault == ENDURANCE_FAULT_OVERCURRENT);

    endurance_tick(&c, 100000u); /* time passing never leaves FAULT */
    CHECK(c.state == ENDURANCE_FAULT);
    CHECK(!endurance_start(&c, &P, 0u));
    CHECK(!endurance_stop(&c));
    CHECK(!endurance_clear_fault(&c, false));
    CHECK(c.state == ENDURANCE_FAULT);

    CHECK(endurance_clear_fault(&c, true));
    CHECK(c.state == ENDURANCE_IDLE);
    CHECK(!endurance_load_command(&c)); /* clearing does not resume the test */
    CHECK(endurance_start(&c, &P, 0u));
    CHECK(c.cycles_completed == 0u && c.fault == ENDURANCE_FAULT_NONE);
}

static void test_fault_from_every_state(void)
{
    endurance_ctx_t c;
    endurance_init(&c);
    endurance_raise_fault(&c, ENDURANCE_FAULT_SENSOR_INVALID); /* BOOT */
    CHECK(c.state == ENDURANCE_FAULT && !endurance_self_test_passed(&c));

    c = idle_ctx();
    endurance_raise_fault(&c, ENDURANCE_FAULT_NONE); /* NONE is mapped, never latched blank */
    CHECK(c.state == ENDURANCE_FAULT && c.fault == ENDURANCE_FAULT_INTERNAL);

    c = idle_ctx();
    endurance_start(&c, &P, 0u);
    endurance_tick(&c, 100u);
    endurance_raise_fault(&c, ENDURANCE_FAULT_OVERTEMPERATURE); /* RUNNING_OFF */
    CHECK(c.state == ENDURANCE_FAULT);

    c = idle_ctx();
    endurance_start(&c, &P, 0u);
    endurance_tick(&c, 100u);
    endurance_tick(&c, 300u);
    endurance_tick(&c, 400u);
    endurance_tick(&c, 600u);
    CHECK(c.state == ENDURANCE_COMPLETE);
    endurance_raise_fault(&c, ENDURANCE_FAULT_OPEN_LOAD); /* COMPLETE */
    CHECK(c.state == ENDURANCE_FAULT);
}

static void test_wraparound(void)
{
    endurance_ctx_t c = idle_ctx();
    const uint32_t t0 = 0xFFFFFFC0u; /* 64 ms before the counter wraps */
    endurance_start(&c, &P, t0);
    endurance_tick(&c, t0 + 99u);
    CHECK(c.state == ENDURANCE_RUNNING_ON);
    endurance_tick(&c, t0 + 100u); /* = 0x24 after wrap */
    CHECK(c.state == ENDURANCE_RUNNING_OFF);
    CHECK(c.phase_start_ms == 0x00000024u);
    endurance_tick(&c, t0 + 299u);
    CHECK(c.state == ENDURANCE_RUNNING_OFF);
    endurance_tick(&c, t0 + 300u);
    CHECK(c.state == ENDURANCE_RUNNING_ON && c.cycles_completed == 1u);
}

static void test_late_tick_neither_replays_nor_shortens(void)
{
    endurance_ctx_t c = idle_ctx();
    endurance_start(&c, &P, 0u);
    endurance_tick(&c, 10000u); /* scheduler stalled for 10 s */
    CHECK(c.state == ENDURANCE_RUNNING_OFF && c.cycles_completed == 0u);
    endurance_tick(&c, 10199u); /* OFF must still last the full 200 ms */
    CHECK(c.state == ENDURANCE_RUNNING_OFF);
    endurance_tick(&c, 10200u);
    CHECK(c.state == ENDURANCE_RUNNING_ON && c.cycles_completed == 1u);
}

static void test_null_safety(void)
{
    endurance_init(NULL);
    CHECK(!endurance_self_test_passed(NULL));
    CHECK(!endurance_start(NULL, &P, 0u));
    CHECK(!endurance_stop(NULL));
    endurance_raise_fault(NULL, ENDURANCE_FAULT_OVERCURRENT);
    CHECK(!endurance_clear_fault(NULL, true));
    endurance_tick(NULL, 0u);
    CHECK(!endurance_load_command(NULL));
}

static void test_debounce(void)
{
    debounce_t d;
    debounce_reset(&d);
    CHECK(!debounce_update(&d, true, 3u));
    CHECK(!debounce_update(&d, true, 3u));
    CHECK(debounce_update(&d, true, 3u));  /* third consecutive true */
    CHECK(debounce_update(&d, true, 3u));  /* stays tripped while true */
    CHECK(d.consecutive == 3u);            /* saturates */
    CHECK(!debounce_update(&d, false, 3u)); /* one false sample clears it */
    CHECK(!debounce_update(&d, true, 3u));
    CHECK(!debounce_update(&d, true, 3u));
    CHECK(!debounce_update(&d, false, 3u)); /* interrupted run restarts */
    CHECK(!debounce_update(&d, true, 3u));
    CHECK(debounce_update(&d, true, 1u));   /* required=1 trips at once */
    debounce_reset(&d);
    CHECK(debounce_update(&d, true, 0u));   /* required=0 treated as 1 */
    CHECK(!debounce_update(NULL, true, 3u));
    debounce_reset(NULL);
}

/* Generated test cases: a fixed-seed pseudo-random operation sequence checked
 * against invariants. Not a substitute for hardware testing. */
static uint32_t lcg(uint32_t *s)
{
    *s = *s * 1664525u + 1013904223u;
    return *s >> 8;
}

static void test_invariants_random_sequences(void)
{
    uint32_t seed = 12345u;
    endurance_ctx_t c;
    endurance_init(&c);
    uint32_t now = 0xFFFFF000u; /* start near wrap to exercise it */
    int ok = 1;

    for (int i = 0; i < 200000; i++) {
        now += lcg(&seed) % 120u;
        switch (lcg(&seed) % 12u) {
        case 0: endurance_self_test_passed(&c); break;
        case 1: {
            endurance_profile_t p = {.on_ms = 10u + lcg(&seed) % 200u,
                                     .off_ms = 10u + lcg(&seed) % 200u,
                                     .target_cycles = 1u + lcg(&seed) % 5u};
            endurance_start(&c, &p, now);
            break;
        }
        case 2: endurance_stop(&c); break;
        case 3:
            if (lcg(&seed) % 8u == 0u) {
                endurance_raise_fault(&c, (endurance_fault_t)(lcg(&seed) % 8u));
            }
            break;
        case 4: endurance_clear_fault(&c, (lcg(&seed) & 1u) != 0u); break;
        default: endurance_tick(&c, now); break;
        }

        if (endurance_load_command(&c) != (c.state == ENDURANCE_RUNNING_ON)) ok = 0;
        if (c.state == ENDURANCE_FAULT && (endurance_load_command(&c) || c.fault == ENDURANCE_FAULT_NONE)) ok = 0;
        if (c.state == ENDURANCE_BOOT && endurance_load_command(&c)) ok = 0;
        if ((c.state == ENDURANCE_RUNNING_ON || c.state == ENDURANCE_RUNNING_OFF ||
             c.state == ENDURANCE_COMPLETE) && c.cycles_completed > c.profile.target_cycles) ok = 0;
        if (c.state == ENDURANCE_COMPLETE && c.cycles_completed != c.profile.target_cycles) ok = 0;
        if (!ok) {
            printf("invariant broken at step %d (state=%d)\n", i, (int)c.state);
            break;
        }
    }
    CHECK(ok);
}

int main(void)
{
    test_profile_validation();
    test_boot_gate();
    test_start_rejects_invalid_profile();
    test_nominal_schedule();
    test_stop_does_not_count_partial_cycle();
    test_fault_latches_and_blocks_restart();
    test_fault_from_every_state();
    test_wraparound();
    test_late_tick_neither_replays_nor_shortens();
    test_null_safety();
    test_debounce();
    test_invariants_random_sequences();

    printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
