# Automated Electrical Load Endurance & Fault-Diagnostic Test Controller

**Platform:** ESP32 (dual-core, FreeRTOS) · **Language:** Embedded C/C++ (ESP-IDF)
**Domain framing:** Automotive-adjacent electrical load test bench (12 V DC loads: relays, solenoids, motors, lamps)

## Scope & Honesty Statement

This project demonstrates embedded firmware architecture, RTOS task design, sensor-based
diagnostics, and CAN-based test communication at a **student bench-test scale**, using a
12 V DC supply and no mains switching. It does **not** claim automotive-grade qualification,
ISO 26262 functional safety compliance, or production readiness. All performance figures in
`docs/validation_report.md` are measurements from this specific bench setup, not
industry-certified accuracy claims.

## What this is

A single-channel (extensible to multi-channel) load endurance and fault-diagnostic
controller. It cycles a 12 V DC load ON/OFF for a configured number of cycles, continuously
monitors current/voltage/temperature, detects five fault classes (overcurrent, short circuit,
open load, overtemperature, undervoltage), classifies faults as transient vs. hard, and
reports status over UART and CAN. See `docs/requirements.md` for the full SR/FR tables.

## Status

🚧 **Phase 0 — Bring-up.** See `docs/roadmap.md` for the full phase plan. Currently proving
individual sensors and peripherals in isolation before any FreeRTOS task structure exists.

## Repository layout

```
docs/           requirements, state machine, hardware architecture, CAN protocol, validation report
firmware/main/  the actual product firmware (built up from Phase 3 onward)
firmware/bringup/  standalone Phase 0 test snippets (blink, ADC, I2C/INA226, DS18B20) — not the product firmware
hardware/       schematic notes + BOM
tools/          PC-side test automation (pyserial)
tests/          fault-injection logs collected during Phase 4/8
```

## Building

Two supported ways to build — pick whichever installs more cleanly on your machine:

**Option A — PlatformIO (recommended on Windows if the raw ESP-IDF installer gives you
pip/Python grief):** install the "PlatformIO IDE" extension in VS Code, then open
`firmware/bringup/` as a PlatformIO project (it contains `platformio.ini`). PlatformIO
downloads its own isolated toolchain — it doesn't touch your system Python/pip.

```bash
cd firmware/bringup
pio run -t menuconfig   # Bring-up Test Selection -> choose BLINK / ADC_READ / I2C_INA226 / DS18B20
pio run -t upload
pio device monitor
```

**Option B — raw ESP-IDF CLI (v5.x):**

```bash
cd firmware/bringup
idf.py set-target esp32
idf.py menuconfig   # Bring-up Test Selection -> choose BLINK / ADC_READ / I2C_INA226 / DS18B20
idf.py build flash monitor
```

Not build-verified in the environment this repo was scaffolded in (no toolchain/network
access here) — verify locally before trusting it compiles either way.

## License

TBD.
