# CLAUDE.md

Guidance for AI assistants and contributors working in this repository.

## What this is

ESP32 / ESP-IDF / FreeRTOS endurance and fault-diagnostic controller for **low-voltage 12 V DC
bench loads**. Current phase: **Phase 0 (bring-up)**. Product firmware (`firmware/main/`) does not
exist yet. Read [docs/traceability.md](docs/traceability.md) for what is actually implemented.

## Layout

| Path | Contents |
|---|---|
| `firmware/bringup/` | Standalone ESP-IDF project, one hardware test selected in menuconfig. **Not** the product firmware. |
| `firmware/core/` | Hardware-independent C99 (scheduler/state machine, debounce). Host-tested; not integrated. |
| `firmware/main/` | Product firmware placeholder (not started) |
| `tests/unit/` | C host tests (`make -C tests/unit`, `make -C tests/unit san`) |
| `tests/python/` | Tests for `tools/bringup_capture.py` |
| `tests/fault_injection_log/` | Real logs only (currently empty) |
| `tools/` | `bringup_capture.py` (serial capture); `pc_test_script.py` is a placeholder |
| `assets/diagrams/` | SVG diagrams + `generate_diagrams.py` (regenerate after edits) |
| `docs/` | Requirements, traceability, audit, architecture, fault policy, CAN, safety, guides |

## Build and test commands

* Bring-up build: `cd firmware/bringup && pio run` (PlatformIO, `espidf`) or `idf.py build` (ESP-IDF >= 5.2). Select the test with menuconfig first. **No build of this code has been verified.**
* Host tests: `make -C tests/unit`, `python3 -m unittest discover -s tests/python`
* Diagrams: `python3 assets/diagrams/generate_diagrams.py`

## Hard rules

1. **Evidence before claims.** Do not mark a requirement, fault, interface or phase as implemented/verified without evidence (test output, build log, bench record). Update [docs/traceability.md](docs/traceability.md) and [docs/validation_report.md](docs/validation_report.md) in the same change. Do not fabricate measurements, results, hardware or accuracy figures.
2. **Keep categories separate**: unit-test simulation, synthetic fixtures, hardware-in-the-loop and bench measurement are different evidence and must be labelled as such.
3. **Low-voltage DC only.** Never add mains (230 V AC) switching or procedures. No automotive-grade, ISO 26262 or certification claims.
4. **Safe state = load OFF.** Firmware must default to OFF at start/reset, on invalid critical measurements and on critical faults; never auto-re-energize a possibly shorted/overheated load without a documented rationale. Firmware OFF is not a substitute for independent hardware protection (fuse/PTC, current-limited supply, hardware trip). The OFF polarity and hardware pull depend on the undecided switching stage - do not assume one.
5. **Unresolved choices stay unresolved** (switching stage, isolation topology, thresholds, resistor/shunt values, CAN pins/bit rate). List them, do not invent values.
6. Do not claim a successful build unless one was run. Do not migrate frameworks. Avoid needless abstraction and doc bloat.
7. Keep `firmware/core` free of ESP-IDF/FreeRTOS/heap dependencies and keep time handling wrap-safe.
8. Do not commit secrets, personal data, or generated build output.
