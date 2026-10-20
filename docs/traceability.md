# Requirements traceability

Authoritative requirement-to-implementation matrix. Requirement text lives in
[requirements.md](requirements.md). **A status is only raised with evidence.**

## Status vocabulary

| Status | Meaning |
|---|---|
| Not started | No code or design artefact |
| In progress | Partial code and/or design exists; requirement not met |
| Implemented, not tested | Code exists; never built or run |
| Unit tested | Host unit tests pass for the logic (no hardware involved) |
| Hardware verified | Demonstrated on real hardware with evidence in the validation report |
| Blocked | Cannot proceed until a design decision is made |

"Unit tested" never implies physical behaviour. **No requirement is currently Hardware verified.**

## System requirements

| ID | Requirement (short) | Status | Source | Tests | Evidence | Limitations | Next action |
|---|---|---|---|---|---|---|---|
| SR1 | Programmable ON/OFF cycling | In progress | `firmware/core/endurance_core.*` | `tests/unit` | Host test run, [validation_report.md](validation_report.md) | Scheduler logic only; no load driver, no timer/task integration | Phase 1 switching hardware, Phase 3 integration |
| SR2 | Persist cycle count (NVS) | Not started | - | - | - | Counter is RAM-only in the core | Design NVS record + write policy (Phase 3) |
| SR3 | Sample V, I, temperature | In progress | `firmware/bringup/main/test_*.c` | none runnable | none | Bring-up reads one sensor at a time; INA226 test reads bus voltage only (no current); **never built or run** | Build + run bring-up (Phase 0), then sensing task (Phase 2) |
| SR4 | Detect 5 fault classes, bounded reaction time | Not started | `firmware/core/debounce.*` (helper only) | `tests/unit` (debounce) | Host test run | No thresholds, no reaction-time budget defined | Define thresholds/budget in [fault_policy.md](fault_policy.md) (Phase 4) |
| SR5 | Safe state + log on fault | In progress | `endurance_core.c` (`endurance_raise_fault`) | `tests/unit` | Host test run | Core latches and commands load OFF logically; no GPIO, no logging, polarity unknown | Phase 1 / Phase 4 |
| SR6 | Transient retry / hard latch | Not started | latch semantics only | `tests/unit` (latch) | Host test run | Retry rules deliberately undefined | Define rationale-based policy first |
| SR7 | UART + CAN reporting | Not started | bring-up `ping`/`pong` only (Implemented, not tested) | - | none | No status/telemetry output, no CAN code | Phase 3 (UART), Phase 6 (CAN) |
| SR8 | Watchdog to safe state | Not started | - | - | - | No tasks exist to supervise | Phase 5 |
| SR9 | Isolate control from load domain | Blocked | - | - | - | Conflicts with "single star ground" and with non-isolated BOM modules (audit A12) | Decide topology: shared ground + protection, or opto/driver isolation |
| SR10 | No mains switching | Documented constraint | README, [safety.md](safety.md) | - | scope statement only | Not a verifiable code property; hardware not built | Keep enforced in review |
| SR11 | No automotive/ISO 26262 claim | Documented constraint | README, safety.md | - | review of docs | - | Keep enforced in review |
| SR12 | Extensible to multi-channel | In progress | `endurance_ctx_t` has no globals | none specific | - | Design intent; no multi-instance test | Add a two-context test when integrating |

## Functional requirements

| ID | Requirement (short) | Status | Source | Tests | Evidence | Limitations | Next action |
|---|---|---|---|---|---|---|---|
| FR1 | Profile config via UART, stored in NVS | In progress | `endurance_profile_validate` | `tests/unit` | Host test run | Validation limits (10 ms - 24 h, cycles >= 1) are provisional software bounds, not load ratings; no UART parser, no NVS | UART command set + NVS (Phase 3) |
| FR2 | Timer-driven cycling, no busy-wait | In progress | `endurance_tick` (non-blocking, wrap-safe) | `tests/unit` incl. wrap-around and late-tick cases | Host test run | Not driven by a real timer/task yet | Integrate into a periodic task (Phase 3) |
| FR3 | Sampling at 100 Hz (V/I) and 1-2 Hz (temp) | Not started | - | - | - | INA226 conversion time/averaging must fit 100 Hz; DS18B20 12-bit conversion takes up to ~750 ms | Phase 2 |
| FR4 | Per-fault rules with debounce | Not started | `debounce.*` | `tests/unit` | Host test run | Per-fault thresholds and N unknown | Phase 4 |
| FR5 | Immediate load OFF, hardware-fast path | Blocked | core (software only) | `tests/unit` | - | No hardware-fast path exists in the design (audit A16) | Decide independent trip (see [hardware_architecture.md](hardware_architecture.md)) |
| FR6 | Recovery policy | Not started | latch + operator clear in core | `tests/unit` | Host test run | No retry/backoff; intentionally | Phase 4 |
| FR7 | Structured UART log + CAN frames | Not started | - | - | - | CAN frames are a draft ([can_protocol.md](can_protocol.md)) | Phase 3 / 6 |
| FR8 | TWDT on control/monitor tasks | Not started | - | - | - | - | Phase 5 |
| FR9 | Test report | Not started | - | - | - | - | Phase 8 |

## Phase 0 deliverables

| Item | Status | Evidence |
|---|---|---|
| Bring-up firmware builds (PlatformIO or ESP-IDF) | **Not verified** - toolchain unavailable to the audit | [validation_report.md](validation_report.md) |
| Blink + UART ping/pong | Implemented, not tested | - |
| ADC divider read | Implemented, not tested | - |
| INA226 register read (+ ID check) | Implemented, not tested | - |
| DS18B20 read | Implemented, not tested | - |
| Serial capture/logging tool | Unit tested (offline fixtures + pty) | `tests/python` |
