# CAN Protocol

_Placeholder — filled in during Phase 6._

## Planned frames

| ID | Name | Rate | Purpose |
|---|---|---|---|
| 0x100 | Status | 10 Hz periodic | State, cycle count, current, voltage, temperature |
| 0x101 | Fault event | Event-triggered | Fault type, severity, measured value, cycle number |
| 0x200 | Command | RX | Start / stop / reset from a CAN host |

Exact byte layout (DLC, byte offsets, scaling) to be defined once Phase 6 hardware bring-up
starts. Consider producing a `.dbc` file alongside this doc for use with SavvyCAN/candump.
