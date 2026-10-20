# Fault-injection logs

Real raw logs from fault-injection runs (Phase 4/8) go here: UART logs, CAN captures, scope
screenshots. **No fault injection has been performed; this directory intentionally holds no
results.**

Do not commit simulated or hand-written numbers here. Unit-test simulations belong in
`tests/unit` / `tests/python` and must stay labelled as simulations.

## Record format

For each run add a `YYYY-MM-DD_<fault>_<NN>.md` next to the raw log containing:

* Hardware: switching stage, load, supply setting (voltage and current limit), fuse/PTC,
  sensor mounting, firmware commit and whether the tree was dirty
* Safety gate: confirmation that the checklist in `docs/safety.md` was completed
* Injection method (how the fault was created, and how power was removed afterwards)
* Pass/fail criteria written *before* the run
* Commands issued and the raw log file name(s) (captured with `tools/bringup_capture.py` where applicable)
* Observations, measured latencies and the instrument used, limitations

Update `docs/validation_report.md` and `docs/traceability.md` only from these records.
