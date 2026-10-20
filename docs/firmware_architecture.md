# Firmware Architecture

**Status:** only Phase 0 bring-up code and a host-tested scheduler core exist. There is **no
product firmware, no FreeRTOS task structure, no NVS, no watchdog use, no CAN code** yet.

## What exists

| Location | Contents | Status |
|---|---|---|
| `firmware/bringup/` | Standalone ESP-IDF project; one of four hardware tests selected in menuconfig. Not the product firmware. | Implemented, not built/run |
| `firmware/core/` | Hardware-independent C99: `endurance_core` (profile validation, state machine, cycle counting, fault latch) and `debounce`. No ESP-IDF/FreeRTOS/heap use. | Unit tested on host; **not integrated into any firmware build** |
| `firmware/main/` | Placeholder README | Product firmware not started |

## Intended module boundaries (proposed, not implemented)

| Module | Responsibility | Exists? |
|---|---|---|
| config / profile | Defaults, limits, versioned NVS record | validation only (core) |
| hal_gpio / load_switch | Drive the switch to a verified OFF level at boot; apply command from the state machine | no |
| meas_current_voltage | INA226 driver (calibration, error handling) and/or ADC read | bring-up reads only |
| meas_temperature | DS18B20 driver with CRC/range handling | bring-up read only |
| meas_validation | Staleness, range, plausibility; produces *valid/invalid*, never silently substitutes | no |
| endurance (state machine) | Pure logic from `firmware/core` | yes (host-tested) |
| fault_detection / fault_manager | Threshold + debounce, classification, recovery policy | debounce helper only |
| uart_cli | Command parser + structured log lines | no (bring-up `ping` only) |
| can_comm | TWAI status/fault/command frames | no |
| persistence | NVS wrapper, write-rate limiting | no |
| watchdog | TWDT registration of supervised tasks | no |
| report | End-of-test summary | no |

These are logical boundaries; collapse them into fewer files while the project is small.

## Proposed task model (for Phase 3+)

Not implemented. Intended: one **control task** (fixed-period tick calling
`endurance_tick`, owns the load output and state), one **measurement task** (owns sensor
buses, publishes validated samples with timestamps through a queue or a mutex-protected
latest-value struct), one **comms task** (UART/CAN, lower priority, never blocks control),
and NVS writes only on state changes / cycle milestones (never per sample). The control
task must react to a fault flag within one tick regardless of the ON/OFF interval, which is
why the scheduler is a periodic `tick(now_ms)` and not a delay loop. Watchdog registration
happens only for tasks that exist and are actually supervised.

## Design rules already followed in `firmware/core`

* Load command is a pure function of state (`true` only in `RUNNING_ON`).
* Elapsed time uses unsigned `uint32_t` subtraction (wrap-safe across ~49.7 days of
  millisecond counter); phase length is capped at 24 h so it stays below 2^31 ms.
* A late tick never replays missed phases and never shortens the next one.
* Counters cannot overflow (`cycles_completed` only increments below `target_cycles`).
* Units are in identifier names (`_ms`); no heap, no strings, no hidden global state.

## Build environments

See [setup.md](setup.md). Bring-up: PlatformIO (`espressif32` / `espidf`) or ESP-IDF >= 5.2.
Core + tests: any C99 compiler on a host.
