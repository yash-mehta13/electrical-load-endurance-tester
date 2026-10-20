# Contributing

This is a hardware-in-the-loop embedded project at an early stage. Honest status matters more
than feature count: read [CLAUDE.md](CLAUDE.md) (the rules apply to everyone) and
[docs/safety.md](docs/safety.md) before touching anything electrical.

## Setup and build validation

Follow [docs/setup.md](docs/setup.md). Before opening a pull request, run what applies:

```bash
make -C tests/unit && make -C tests/unit san
python3 -m unittest discover -s tests/python
cd firmware/bringup && pio run          # or idf.py build; say which toolchain/version you used
```

State in the PR what you could **not** run (for example, no board available).

## Adding sensor support

1. Record the part, interface, supply, address/pin and failure modes in [docs/hardware_architecture.md](docs/hardware_architecture.md).
2. Add a bring-up test under `firmware/bringup/` (Kconfig entry, conditional source in `main/CMakeLists.txt`, dispatcher in `app_main.c`) before any product driver.
3. Handle init failure, bus errors, stale/out-of-range data and report *invalid* explicitly; never substitute a plausible number.
4. Keep raw values, calibrated values, thresholds and decisions separate; put units in names.
5. Do not state an accuracy until compared with a reference instrument.

## Changing the state machine

Edit `firmware/core/endurance_core.[ch]`, add/adjust tests in `tests/unit/`, update
[docs/state_machine.md](docs/state_machine.md) and the diagram generator, then regenerate the SVG.
Invariants that must hold: load ON only in `RUNNING_ON`; no start before self-test or from FAULT;
partial cycles are never counted; no automatic restart after a latched fault; no long blocking
delay as the scheduler.

## Changing the CAN protocol

Update [docs/can_protocol.md](docs/can_protocol.md) first (IDs, DLC, byte offsets, scaling, byte
order, enums, rates). Keep *draft* and *implemented* clearly separate. Add a DBC only for frames
the firmware actually sends, and only claim CAN verified with a capture from an independent tool.

## Contributing tests

Label every test: unit/simulation, synthetic fixture, hardware-in-the-loop, or bench measurement.
Never present simulated values as measurements.

## Recording hardware results

Follow the record format in [tests/fault_injection_log/README.md](tests/fault_injection_log/README.md): hardware setup, safety-gate
confirmation, pass/fail criteria written before the run, commands, raw logs (`tools/bringup_capture.py`),
limitations. Then update the validation report and traceability matrix from that record only.
Do not include personal data or credentials in logs.

## Commit hygiene

Small, focused commits; describe the evidence for any status change. Changelog entries in
[CHANGELOG.md](CHANGELOG.md) must describe real changes only.
