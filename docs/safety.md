# Safety, scope and limitations

## Scope

* Low-voltage **12 V DC** bench loads (relay, solenoid, small motor, lamp) fed from a
  **current-limited** laboratory supply.
* **No mains (230 V AC) switching and no mains test procedures** belong in this project.
* This is an educational/portfolio architecture. It is **not** automotive-grade, not
  production-qualified, not certified, and makes no ISO 26262 or other functional-safety
  claim. Measurements in the validation report describe one bench setup only.

## What software cannot do

* A firmware load-OFF is not a substitute for independent hardware protection. A short
  circuit can damage a switch faster than any sampled measurement and control loop.
* Detecting a fault in software is not protecting the circuit.
* ESP32 GPIOs are undefined during reset; the safe (OFF) level must be enforced by a
  hardware pull on the control line.
* Sensor readings are not accurate until compared against a reference; datasheet typical
  values do not describe the assembled measurement chain.

## Gate before any powered endurance run

Every item must be checked and recorded by the engineer (add the record to the validation report):

- [ ] Switching stage chosen; ratings (voltage, current, SOA, temperature) written down and
      above the load's worst case (including inrush and motor stall)
- [ ] Control line has a hardware pull to the OFF level; OFF behaviour verified at power-up and reset with the load disconnected or replaced by a safe dummy
- [ ] Bench supply current limit set below the wiring/switch limit; voltage set to 12 V before connecting
- [ ] Fuse/PTC fitted with a rating justified for the loop; wire gauge and connectors suitable
- [ ] Flyback diode/TVS fitted for inductive loads; INA226 and ADC inputs protected from spikes
- [ ] Independent short-circuit/overcurrent protection in place **or** the risk explicitly accepted with a limited-energy supply setting
- [ ] DS18B20 mounted on the switch/heatsink; heatsinking adequate for the duty cycle
- [ ] Every sensor reading checked for plausibility against a meter/thermometer at no load
- [ ] A physical way to remove load power quickly (supply output switch or unplug) within reach
- [ ] Safe start-up after reset confirmed; first runs are short and attended
- [ ] No unattended or long-duration run until the above plus a short attended run are recorded

If any assumption is unverified, treat it as unsafe.

## Retry behaviour

The design must not automatically re-energize a possibly shorted, overheated or unsafe load.
See [fault_policy.md](fault_policy.md).

## Reporting issues

Safety-relevant concerns: see [SECURITY.md](../SECURITY.md).
