# Product firmware - not started

No product firmware exists yet; this directory is intentionally empty of code. Product firmware
(control task, measurement task, UART/CAN, NVS, watchdog) starts here from Phase 3 onward, built
around the host-tested scheduler in [`../core`](../core/README.md). Phase 0 bring-up code lives
in [`../bringup`](../bringup/); see [`docs/firmware_architecture.md`](../../docs/firmware_architecture.md).
