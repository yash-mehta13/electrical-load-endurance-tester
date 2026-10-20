# Testing guide

Test evidence is kept in four clearly separate categories. Never mix them in a report.

| Category | Meaning | Exists today |
|---|---|---|
| **Host unit tests** | Pure software logic on a PC | Yes (`tests/unit`, `tests/python`) |
| **Synthetic fixtures / generated cases** | Hand-written log lines, fixed-seed pseudo-random sequences | Yes, labelled in the test files |
| **Hardware-in-the-loop** | Real ESP32, simulated or dummy inputs | No |
| **Bench measurements** | Real ESP32 + real sensors/loads, with reference instruments | No |

## Host unit tests

```bash
make -C tests/unit               # build + run; exit status 0 = all checks passed
make -C tests/unit san           # with -fsanitize=address,undefined
CC=clang make -C tests/unit clean test
```

Covers `firmware/core`: profile validation limits, start gating (no start before self-test,
none from FAULT), exact ON/OFF boundaries, cycle counting (only after the full OFF interval),
stop/fault never counting a partial cycle, latched fault semantics and authorized clear,
`uint32_t` wrap-around, late ticks, NULL handling, debounce behaviour, and a fixed-seed
200k-step invariant check (load ON only in RUNNING_ON, no restart out of FAULT, counters
never exceed the target).

These tests say nothing about real switching, timing accuracy of a real timer, sensor
accuracy, or fault reaction time.

## Python tests

```bash
python3 -m unittest discover -s tests/python -v
```

Parser/CLI tests use **synthetic** console lines written from the firmware's format strings,
not captured output. The serial-path tests run against a pseudo-terminal and are skipped if
`pyserial` is not installed.

## Serial capture of a bring-up run (needs the board)

```bash
pip install -r tools/requirements.txt
mkdir -p tests/bringup_logs
python3 tools/bringup_capture.py --list-ports
python3 tools/bringup_capture.py --test ds18b20 --port /dev/ttyUSB0 --duration 20 \
    --reference 23.5 --tolerance 2.0 --note "thermometer: <model>" \
    --out tests/bringup_logs/2026-01-01_ds18b20.jsonl
```

* `--test`: `blink` (sends `ping`, expects `pong`), `adc`, `ina226`, `ds18b20`.
* `--reference/--tolerance`: **you** supply an independent reference reading (multimeter,
  thermometer) and an acceptable error. Without them the verdict is `not_evaluated`; the tool
  never invents thresholds.
* `--no-reset` avoids resetting the ESP32 when the port opens. `--replay FILE` parses a saved
  monitor log offline.
* Output is JSON lines: a `meta` record (tool/Python/pyserial versions, repo commit and dirty
  flag, port, baud, parameters, operator note), `sample`/`log` records with host timestamps,
  and a `summary` record. Exit codes: 0 ok / not evaluated, 1 check failed, 2 usage or port
  error, 3 no samples, 4 serial failure mid-capture.

Commit real logs under `tests/` together with an entry in the validation report.

## Physical tests

1. Complete the gate in [safety.md](safety.md).
2. Start with no load: confirm every sensor reads plausibly against a reference.
3. Confirm the OFF state at power-up and reset before connecting any load.
4. Short, attended runs first; record exact supply setting, load, ambient temperature,
   commands used, observations and the raw log.
5. Fault injection (open load, overload, over-temperature, undervoltage) only with the
   protection in place. Short-circuit tests must not be run until independent hardware
   protection is verified. Save raw logs under `tests/fault_injection_log/` (see its README).

Pass/fail criteria must be written down **before** the run.
