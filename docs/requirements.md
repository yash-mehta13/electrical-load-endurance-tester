# Requirements

## System Requirements

| # | Requirement | Type |
|---|---|---|
| SR1 | Switch a 12 V DC load ON/OFF on a programmable duty cycle (ON time, OFF time, total cycle count) | Functional |
| SR2 | Count completed cycles and persist count across resets (NVS) | Functional |
| SR3 | Continuously sample load voltage, load current, and switch/heatsink temperature | Functional |
| SR4 | Detect open-load, short-circuit, overcurrent, overtemperature, undervoltage faults within a bounded reaction time | Functional |
| SR5 | On fault, transition to a safe state (load OFF) and log fault type, timestamp, cycle number, and measured value | Functional |
| SR6 | Support fault classification with recovery attempts for transient faults and latch-off for hard faults | Functional |
| SR7 | Report status/telemetry over UART (human-readable log) and CAN (structured frames) | Functional |
| SR8 | Detect firmware hangs via watchdog and reset to a known safe state | Functional |
| SR9 | Physically isolate low-voltage ESP32 control logic from the 12 V switched load path | Non-functional / Safety |
| SR10 | No mains (230 V AC) switching anywhere in the design | Non-functional / Safety |
| SR11 | System must not claim ISO 26262 / automotive-grade compliance — demonstrates the *architecture*, not certified safety | Non-functional |
| SR12 | Firmware must be modular enough to add channels (multi-load) without rewriting the core state machine | Non-functional |

## Functional Requirements (Detailed)

- **FR1 – Test Profile Configuration:** ON duration, OFF duration, target cycle count, overcurrent threshold, overtemperature threshold, undervoltage threshold configurable via UART command interface before a test starts (stored in NVS).
- **FR2 – Endurance Cycling:** Firmware drives a high-side switch through N programmable ON/OFF cycles, incrementing a persistent cycle counter, without CPU busy-waiting (timer-driven).
- **FR3 – Continuous Monitoring:** Voltage, current, and temperature sampled at a fixed rate (100 Hz for current/voltage, 1–2 Hz for temperature) independent of the ON/OFF cycle timing.
- **FR4 – Fault Detection:** Each fault class has its own detection rule and debounce/consecutive-sample requirement:
  - Overcurrent: measured current > threshold for ≥ N consecutive samples
  - Short circuit: current above a much higher instantaneous threshold, immediate (< 1 sample debounce)
  - Open load: load commanded ON but current stays near-zero for > timeout
  - Overtemperature: switch/heatsink temperature > threshold, with hysteresis for recovery
  - Undervoltage: supply rail sags below threshold while load is ON
- **FR5 – Fault Response:** Immediately command load OFF (hardware-fast path, not just software), log the event, move to FAULT state.
- **FR6 – Recovery Policy:** Transient faults get limited auto-retry with backoff; hard faults latch off and require manual/UART-command reset.
- **FR7 – Logging:** UART emits structured log lines. CAN emits periodic status frames and event-triggered fault frames.
- **FR8 – Watchdog:** Task Watchdog Timer (TWDT) on control/monitor tasks; on hang, ESP32 resets and firmware recovers to a safe (load-OFF) state, logging the reset reason.
- **FR9 – Test Report:** At completion/abort: total cycles completed, total runtime, fault counts by class, min/max/avg current & temperature.
