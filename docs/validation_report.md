# Validation Report

**Principle: measured results only.** No bench measurement, build log, endurance run, fault
injection or CAN capture exists yet. The measurement tables below are intentionally empty;
do not fill them from simulation, datasheets or expectation.

Evidence categories are kept apart ([testing.md](testing.md)): host unit tests / synthetic
fixtures (section 1), build attempts (section 2), bench measurements (sections 3-4).

## 1. Software checks actually run (host only, 2026-10-20)

Repository base commit `6fb7321` plus the uncommitted changes of this work. Environment: Linux, Python 3.13.16,
gcc 13.3.0, clang 18.1.3, GNU Make 4.3, pyserial 3.5.

| Check | Command | Result |
|---|---|---|
| Scheduler/debounce core, gcc | `make -C tests/unit` | 97 checks, 0 failures |
| Same, AddressSanitizer + UBSan | `make -C tests/unit san` | 97 checks, 0 failures, no sanitizer reports |
| Same, clang | `make -C tests/unit CC=clang clean test` | 97 checks, 0 failures |
| Capture-tool tests | `python3 -m unittest discover -s tests/python` | 19 tests OK (replay parser/CLI on **synthetic** lines; serial path against a pseudo-terminal incl. split writes, ping/pong, simulated unplug, missing port) |
| Python syntax | `python3 -m py_compile tools/*.py assets/diagrams/generate_diagrams.py` | OK |
| SVG well-formedness | `xmllint --noout assets/diagrams/*.svg` | all 6 OK; also rendered with headless Chromium and inspected for legibility/overlaps |
| Mermaid block in `docs/state_machine.md` | rendered with Mermaid 10.9.1 in headless Chromium | rendered without error |
| Markdown links/paths | script over all `*.md` | 95 relative links, 0 broken; all backticked repo paths exist |
| BOM CSV | `csv` parse | 16 rows, 5 columns each (a malformed row was fixed) |
| Mutation sanity check | two defects injected into the scheduler (late phase boundary, cycle counted at end of ON) | both detected by the tests; code restored |

These checks demonstrate **logic behaviour on a PC only**. They are not evidence of
physical switching, timing accuracy of a real timer, sensor accuracy, fault reaction time,
watchdog behaviour or CAN operation.

## 2. Build verification of the bring-up firmware

| Toolchain | Result |
|---|---|
| PlatformIO Core 6.2.0 (installed with pip) `pio run` on the **original** project | **Failed to start:** the PlatformIO registry host was blocked by the sandbox network policy (HTTP 403 at the proxy), so the `espressif32` platform could not be installed |
| ESP-IDF (`idf.py`) | Not installed; Espressif docs/registry hosts unreachable from the sandbox |
| Modified bring-up sources | Syntax-only check (`gcc -fsyntax-only`) against **hand-written stub headers** whose signatures were checked against ESP-IDF v5.2 headers fetched from GitHub. This catches typos only; it is **not** a build and says nothing about linking, Kconfig or component resolution |

**Status: the bring-up firmware has not been built.** Before the changes the code very probably
did not build either (audit A1, A2, found by inspection). The first real build should be
recorded here with PlatformIO/IDF versions and the full log.

## 3. Phase 0 hardware gate (not run)

| Gate item | Status | Why |
|---|---|---|
| Toolchain builds and flashes | Not run | No toolchain access / no board |
| Blink + UART ping/pong | Not run | No board |
| ADC divider read vs meter | Not run | No board, divider values unrecorded |
| INA226 ID + bus voltage vs meter | Not run | No board/module |
| DS18B20 vs thermometer | Not run | No board/sensor |
| 12 V supply current limit checked | Not run | No bench access |

## 4. Test conditions and metrics (to be filled with real data)

Test conditions

| Item | Value |
|---|---|
| Supply voltage / current limit | |
| Load type and rating | |
| Switching stage and protection fitted | |
| Ambient temperature | |
| Firmware commit / toolchain versions | |
| Instruments used (with calibration status) | |

Metrics

| Metric | Result | Evidence (log/file) |
|---|---|---|
| Fault detection latency (per class) | | |
| False-positive rate (N hours clean cycling) | | |
| Cycle-count accuracy vs independent reference | | |
| Endurance run - total cycles / runtime | | |
| Recovery behaviour (policy not yet defined) | | |
| CAN bus health (bus-off events, frame jitter) | | |
| Watchdog verification (hang -> recovery time) | | |
| Sensor error vs reference (V, I, T) | | |

## 5. Limitations of this report

Nothing in sections 3-4 has been executed. Datasheet-derived statements elsewhere in the
repository (INA226 registers and limits, DS18B20 behaviour, BTS7960/SN65HVD230 features) were
written from memory because manufacturer datasheet hosts were unreachable, and must be
re-checked against current datasheets.
