# firmware/core - hardware-independent logic

Plain C99 with no ESP-IDF, FreeRTOS, heap or I/O dependencies, so it can be unit-tested on
a PC.

| File | Purpose |
|---|---|
| `endurance_core.[ch]` | Test-profile validation, endurance state machine, cycle counting, fault latch |
| `debounce.[ch]` | Consecutive-sample qualifier |

**Status: unit tested on a host only** (`make -C tests/unit`). It is not part of any
firmware build and has never controlled real hardware. Provisional values (phase limits,
fault enumeration) are marked in the headers. Behaviour is documented in
[`docs/state_machine.md`](../../docs/state_machine.md).
