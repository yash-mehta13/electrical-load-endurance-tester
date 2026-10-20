# Changelog

Only real changes are recorded. There are no releases or version tags yet.

## Unreleased

### Added
- `firmware/core`: hardware-independent endurance scheduler/state machine and debounce helper, with host unit tests (`tests/unit`).
- `tools/bringup_capture.py`: serial capture of bring-up output to JSON lines with optional reference check; Python tests (`tests/python`).
- Six SVG diagrams and their generator (`assets/diagrams/`).
- Documentation: audit, traceability matrix, firmware architecture, fault policy, safety, setup, testing, troubleshooting; CAN protocol draft; expanded roadmap with phase gates.
- `CLAUDE.md`, `CONTRIBUTING.md`, `SECURITY.md`, issue and pull-request templates.

### Changed
- Bring-up firmware: compile only the selected test (previously all four were compiled against Kconfig symbols that exist only for the selected one); derive the ADC channel from the GPIO; choose the ADC calibration scheme by chip support (curve fitting is unavailable on ESP32); divider resistors via Kconfig; require ESP-IDF >= 5.2 at compile time; log and continue on sensor errors; flag the DS18B20 85.0 C reset value; read INA226 ID registers; non-blocking UART console polling in the blink test; print reset reason and IDF version. **None of this has been built or run on hardware.**
- `README.md` rewritten to reflect the actual Phase 0 state; `docs/hardware_architecture.md`, `docs/state_machine.md`, `docs/can_protocol.md`, `docs/roadmap.md`, `docs/validation_report.md` rewritten or extended; `docs/requirements.md` gained an open-issues note.
- `hardware/bom.csv`: quoted a malformed row; softened unverified BTS7960 protection claim; noted missing shunt/divider values and DS18B20 pull-up.
- `.gitignore`: also ignore generated `sdkconfig.*` (keeping `sdkconfig.defaults*`).

### Not done
- No ESP32 build, no hardware verification, no license selected.
