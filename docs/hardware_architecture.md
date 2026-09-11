# Hardware Architecture

## Block Diagram

```
 [12V Bench PSU] ──┬─────────────────────────────► [12V DC Load: relay/motor/lamp]
                    │                                        ▲
                    │                              High-side switch (drain/source)
                    │                                        │
              [Shunt/Hall Current Sensor] ───────────────────┘
                    │
                    ├──► [Voltage Divider → ADC]     (load-side voltage sense)
                    ├──► [Current Sensor Amp → ADC]  (INA226)
                    ├──► [DS18B20 Digital Temp Sensor] (on heatsink/switch)
                    │
        ┌───────────▼───────────────────────────────────────────────┐
        │                 ESP32 (3.3V logic domain)                 │
        │  ADC in │ I2C (INA226/temp) │ GPIO out (gate drive via     │
        │         │                   │ opto/level shifter)          │
        │  UART (logging/console)     │ TWAI/CAN controller pins     │
        └──────────────────────────────────────────────────────────┘
                    │                                        │
             [SN65HVD230 CAN Transceiver] ──► CAN bus (to PC CAN tool / second node)
```

## Component Roles

| Component | Role |
|---|---|
| ESP32 (DevKit) | Main controller — FreeRTOS tasks for cycling, sensing, diagnostics, comms |
| High-side smart switch (BTS7960) or MOSFET + gate driver | Switches the 12 V load ON/OFF under firmware control |
| INA226 | I2C current/voltage sensing |
| DS18B20 | Digital temperature sensing on the switch/heatsink |
| SN65HVD230 | 3.3 V CAN (TWAI) transceiver |
| Flyback diode | Protects against inductive kickback from relay/motor/solenoid loads |
| Fuse/PTC | Load-line overcurrent protection independent of firmware |

Full BOM with pricing: `hardware/bom.csv`. Schematic to be added under `hardware/schematic/`
once Phase 1 wiring is finalized.

## Isolation boundary (SR9)

Control domain (3.3–5 V, ESP32 + logic) and load domain (12 V switched path) are kept
physically separate — single star-ground point, opto/level-shifter boundary if using a
discrete MOSFET. To be photographed and documented here once Phase 1 hardware is built.
