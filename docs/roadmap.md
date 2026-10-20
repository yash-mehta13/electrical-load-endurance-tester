# Implementation Roadmap

Staged development. A phase is complete only when its **gate** is met with evidence recorded in
[validation_report.md](validation_report.md) and [traceability.md](traceability.md).
Durations are the original estimates.

| Phase | Focus | Duration | Status |
|---|---|---|---|
| 0 | Bring-up & procurement | 3-4 days | **In progress** - code written, nothing built or run on hardware |
| 1 | Core switching hardware | 4-5 days | Not started |
| 2 | Sensing pipeline | 4-5 days | Not started |
| 3 | State machine (no faults yet) | 4-5 days | Core logic host-tested; no integration |
| 4 | Fault diagnostics | 6-8 days | Not started |
| 5 | Watchdog & robustness | 2-3 days | Not started |
| 6 | CAN integration | 5-6 days | Not started |
| 7 | PC-side test automation | 3-4 days | Bring-up capture tool only |
| 8 | Long-run validation & docs | 5-7 days | Not started |

## Phase 0 - Bring-up & procurement (current)

**Goal:** prove every individual piece of hardware works in isolation before combining anything.

- [ ] Finalize BOM and order parts (gaps listed in [audit.md](audit.md) A15)
- [ ] Bench-test the 12 V supply, confirm current limiting works
- [ ] Toolchain builds and flashes the bring-up project (record versions)
- [ ] Blink + UART console skeleton (ping/pong)
- [ ] ADC read (voltage divider) - plausible reading against a meter
- [ ] I2C read (INA226 ID + bus voltage) - plausible reading
- [ ] DS18B20 standalone test - plausible reading at room temperature (the only defined temperature sensor; NTC is not selected)

**Entry:** parts in hand. **Gate:** every sensor gives a plausible reading on the bench, the ESP32 boots reliably, logs saved with `tools/bringup_capture.py`.

## Phase 1 - Core switching hardware

**Entry:** Phase 0 gate; switching-stage decision (BTS7960 module vs MOSFET + driver), ground/isolation decision (SR9), fuse/PTC and flyback/TVS ratings chosen.
**Work:** assemble switch with hardware pull to OFF level; drive from a GPIO; decide on an independent overcurrent/short-circuit trip; draw the schematic.
**Gate:** OFF level at power-up and reset verified with a scope/meter; a resistive dummy load switches repeatedly; protection behaviour demonstrated with current-limited supply; [safety.md](safety.md) gate list complete.

## Phase 2 - Sensing pipeline

**Entry:** Phase 1 gate. **Work:** INA226 calibration (real shunt), divider values, DS18B20 driver with CRC/range handling, measurement validator, fixed sampling rates (100 Hz V/I, 1-2 Hz T), authoritative voltage source chosen.
**Gate:** readings compared with reference instruments over the working range; error characterised and recorded; invalid-sample handling exercised.

## Phase 3 - State machine integration

**Entry:** Phase 2 gate; `firmware/core` host tests green. **Work:** periodic control task around `endurance_tick`, UART command set (status/configure/start/stop/clear/report/help), NVS versioned profile + counter write policy, safe start after reset.
**Gate:** timing of ON/OFF measured against an independent reference; cycle count verified; reset/power-loss behaviour verified.

## Phase 4 - Fault diagnostics

**Entry:** Phase 3 gate; thresholds derived from chosen hardware ratings. **Work:** implement [fault_policy.md](fault_policy.md) per fault with debounce, hysteresis, evidence record; classification rules only with a written rationale.
**Gate:** each fault injected safely with protection in place; latency measured; false-positive behaviour documented; unit tests for every rule.

## Phase 5 - Watchdog & robustness

**Entry:** Phase 4 gate and real tasks to supervise. **Work:** TWDT registration of control/measurement tasks, brownout behaviour, sensor-loss handling.
**Gate:** a deliberately hung task produces a documented reset to a load-OFF state, with recovery time measured.

## Phase 6 - CAN integration

**Entry:** Phase 3 gate; CAN pins/bit rate/termination/adapter decided. **Work:** TWAI driver, frames from [can_protocol.md](can_protocol.md) (finalise it), DBC, bus-off handling.
**Gate:** capture from an independent tool decodes every frame correctly; bus-off recovery demonstrated; protocol and DBC updated to match.

## Phase 7 - PC-side test automation

**Entry:** UART command set exists. **Work:** extend `tools/` to drive a test profile and log results as JSON/CSV (Phase 0 capture tool exists).
**Gate:** an unattended scripted short run reproduces its log and handles a disconnected device.

## Phase 8 - Long-run validation & documentation

**Entry:** all prior gates. **Work:** endurance runs, false-positive rate, cycle-count accuracy, CAN health, watchdog verification; fill the validation report with real numbers only.
**Gate:** [validation_report.md](validation_report.md) metrics populated from evidence; limitations stated.
