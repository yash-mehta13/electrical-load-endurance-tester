# Hardware Architecture

**Status: design intent. No hardware has been assembled or verified in this repository.**
Items marked **OPEN** are decisions that have not been made; this document does not make them.

![System architecture](../assets/diagrams/system_architecture.svg)

Pin assignments: ![Phase 0 default pin map](../assets/diagrams/pin_map.svg)

## Scope

Single channel, low-voltage 12 V DC bench loads (relay, solenoid, small motor, lamp) from a
current-limited bench supply. No mains. See [safety.md](safety.md).

## Signal and power paths

| Path | Intended signal | Interface | Status |
|---|---|---|---|
| Power | Bench PSU -> fuse/PTC -> switching stage -> load -> return | 12 V DC | Design intent; ratings OPEN |
| Switch control | ESP32 GPIO -> switching stage enable/gate | 3.3 V logic | GPIO and polarity OPEN |
| Current + bus voltage | INA226 shunt/bus inputs -> ESP32 | I2C (0x40 default; SDA 21 / SCL 22 are bring-up defaults) | Bring-up read code exists; unverified |
| Load voltage (alt.) | Resistor divider -> ESP32 ADC1 | Analog (GPIO34 bring-up default) | Resistor values OPEN |
| Temperature | DS18B20 on switch/heatsink -> ESP32 | 1-Wire (GPIO4 bring-up default) + 4.7k pull-up | Pull-up not in BOM |
| Console | USB-serial bridge on the DevKit | UART0, 115200 | Used by bring-up code |
| CAN | ESP32 TWAI -> SN65HVD230 -> bus | CAN | Planned; pins OPEN ([can_protocol.md](can_protocol.md)) |

## Components and what is and is not established

| Component | Role | Established | OPEN / caution |
|---|---|---|---|
| ESP32 DevKit (WROOM-32, 38-pin) | Controller | Selected in BOM | Exact board variant/pinout to be recorded |
| BTS7960 module **or** N-MOSFET + driver | Load switch | Both listed; BOM calls the module "preferred" | **Not decided.** BTS7960 is an integrated half-bridge (high- and low-side FET) rather than a high-side-only smart switch: using it as a load switch means choosing which output and enable inputs are the safe (off) state and verifying them on the actual module. Its internal protection thresholds and latch/restart behaviour come from the Infineon datasheet and have not been checked here. A discrete MOSFET design needs gate-driver, gate pull-down, SOA and thermal analysis. |
| INA226 module | Current + bus voltage | Selected | Shunt value on the purchased module is not recorded. Datasheet full-scale shunt voltage is +/-81.92 mV (from memory, not re-verified), so max measurable current = 81.92 mV / R_shunt; a motor's stall current may exceed it. Bus-voltage input is rated to 36 V: inductive spikes need clamping. Calibration register and current LSB are not yet configured in firmware. |
| Voltage divider + ADC | Load/supply voltage | Topology only | Resistor values, tolerance, maximum sensed voltage OPEN. The ADC pin must never see more than the ESP32 allows; ESP32 ADC is nonlinear and its eFuse calibration (line fitting) reduces but does not remove error. |
| DS18B20 | Switch/heatsink temperature | Selected (only temperature sensor in BOM and code); NTC **not** selected | Reads case/heatsink temperature, **not** a MOSFET junction temperature. Mounting method OPEN. Reset value 85.0 C; 12-bit conversion up to ~750 ms. |
| SN65HVD230 module | CAN transceiver | Selected | Many modules include a 120 ohm termination: check. Standby/slope pin handling OPEN. |
| Flyback diode / TVS | Inductive kick-back | Diode in BOM (1N5408 class) | Ratings and TVS need for INA226 input OPEN |
| Fuse / PTC | Overcurrent protection independent of firmware | Fuse holder + blade fuses in BOM | **Rating not defined**; PTC not in BOM |

## Measurement notes

* **Two voltage sources** exist (divider/ADC and INA226 bus voltage). Which is authoritative
  for undervoltage decisions, and whether they are cross-checked, is OPEN.
* **Accuracy:** device datasheet figures do not describe the assembled measurement chain
  (shunt tolerance, layout, ADC nonlinearity, divider tolerance, calibration). No accuracy is
  claimed until compared with a reference ([validation_report.md](validation_report.md)).
* **Voltage sag vs undervoltage:** the load's own current will sag the rail through supply
  and wiring resistance. The detection rule must account for it (e.g. evaluate against
  expected sag, or measure at the source side); not yet defined.

## Safe state and power-up behaviour

The firmware default is *load OFF*. Firmware alone cannot guarantee it: ESP32 GPIOs are not
driven to a defined level during reset/boot (and GPIO0/2/5/12/15 are strapping pins). The
switching stage therefore **requires a hardware pull to its OFF level** on the control line,
chosen after the switch topology and polarity are known (OPEN). Until then no firmware may
assume a particular polarity, and none is committed to the repository.

## Protection and isolation limitations

* **Short circuit:** software sampling (even at 100 Hz) is far too slow to protect a
  semiconductor switch. Protection relies on the supply's current limit, a fuse/PTC and, for
  option A, whatever the BTS7960 provides. Whether an independent fast hardware trip
  (comparator/INA226 ALERT-driven or dedicated) is required is OPEN and is expected to be
  *yes* for any load whose fault current could damage the switch before a fuse opens. The
  INA226 ALERT output is limited by its conversion time and is not a certified protection
  path.
* **Isolation (SR9):** not established. The BTS7960 and INA226 modules are not isolating
  parts, a shared ground makes the "isolation" an assumption rather than a barrier, and the
  earlier text ("single star ground") contradicts galvanic isolation. Decide between
  (a) shared ground with documented protection, or (b) opto/gate-driver isolation with
  separate supplies - then update SR9. Until then **no isolation is claimed.**
* **Connectors, copper, wiring** have no current ratings recorded; breadboards are not
  suitable for load current.

## Open decisions checklist

- [ ] Switching stage (A or B) and its safe/OFF polarity
- [ ] Control GPIO + hardware pull level
- [ ] Fuse/PTC rating; TVS/flyback ratings
- [ ] INA226 shunt value, tap location, calibration
- [ ] Divider resistors and maximum sensed voltage
- [ ] Authoritative voltage source
- [ ] DS18B20 mounting
- [ ] Ground/isolation topology
- [ ] CAN pins, bit rate, termination, adapter model
- [ ] Test-load ratings (incl. motor stall current)
- [ ] Independent hardware overcurrent/short-circuit trip: needed? design?

Full BOM: [`hardware/bom.csv`](../hardware/bom.csv). Schematic: not yet drawn (`hardware/schematic/`).
