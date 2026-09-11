# Implementation Roadmap

| Phase | Focus | Duration |
|---|---|---|
| 0 | Bring-up & procurement | 3–4 days |
| 1 | Core switching hardware | 4–5 days |
| 2 | Sensing pipeline | 4–5 days |
| 3 | State machine (no faults yet) | 4–5 days |
| 4 | Fault diagnostics | 6–8 days |
| 5 | Watchdog & robustness | 2–3 days |
| 6 | CAN integration | 5–6 days |
| 7 | PC-side test automation | 3–4 days |
| 8 | Long-run validation & docs | 5–7 days |

## Phase 0 — Bring-up & Procurement (current phase)

**Goal:** Prove every individual piece of hardware works in isolation before combining anything.

- [ ] Finalize BOM and order parts (see `hardware/bom.csv`)
- [ ] Bench-test the 12 V supply, confirm current limiting works
- [ ] `idf.py create-project`, confirm toolchain builds and flashes
- [ ] Blink + UART console skeleton
- [ ] ADC read (raw voltage divider) standalone test — plausible reading at room temp
- [ ] I2C read (INA226 register read) standalone test — plausible reading
- [ ] DS18B20/NTC standalone test — plausible reading at room temperature

**Gate:** every sensor gives a plausible reading on the bench, ESP32 boots reliably.

See the full 9-phase roadmap (with hardware + software tracks, gates, and sequencing notes)
in the project chat history / design doc — this file tracks Phase 0 checklist progress only
for now and will be expanded phase-by-phase as the build proceeds.
