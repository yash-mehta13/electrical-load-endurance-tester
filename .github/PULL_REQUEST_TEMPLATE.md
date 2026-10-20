## Summary

<!-- What changed and why -->

## Evidence

- [ ] `make -C tests/unit` / `make -C tests/unit san`
- [ ] `python3 -m unittest discover -s tests/python`
- [ ] Bring-up build (`pio run` or `idf.py build`) - toolchain and version: <!-- or "not run: reason" -->
- [ ] Hardware test - record link/path: <!-- or "none" -->

Not run, and why:

## Status claims

- [ ] `docs/traceability.md` updated for any status change (with evidence)
- [ ] No simulated value is presented as a measurement
- [ ] No new safety, accuracy, CAN, watchdog or endurance claim without evidence

## Safety

- [ ] Safe state (load OFF) behaviour unaffected, or the change is explained
- [ ] Still low-voltage DC only; no mains
- [ ] Unresolved hardware choices not silently decided

<!-- Please do not include secrets or personal data. -->
