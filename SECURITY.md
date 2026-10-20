# Security and safety reporting

This is a personal/educational embedded project with no established security programme, no
supported releases and no certification of any kind.

## Reporting

* **Security problems** (for example a way for an untrusted serial/CAN input to put the
  controller into an unsafe state, or leaked credentials): please report privately using
  GitHub's "Report a vulnerability" feature on the repository's Security tab if it is
  enabled, otherwise open an issue that does **not** include exploit details and ask for a
  private channel.
* **Safety-relevant defects** (wrong safe-state behaviour, a path that could keep a load
  energized after a fault, misleading electrical documentation): open an issue using the bug
  template and mark it safety-relevant.

Responses are best-effort. Do not post secrets, personal data or credentials in issues or logs.

## Why this matters here

The firmware is intended to switch electrical loads. A defect in switching control,
fault handling or the documentation can lead to overheating, equipment damage or injury even at
12 V (battery-class supplies can deliver large fault currents). Treat everything here as
unverified experimental work: it has not been tested on hardware, is **not** certified, and
must not be used to protect people or property. Software fault handling never replaces
independent hardware protection ([docs/safety.md](docs/safety.md)).

## Scope of input handling

Planned serial and CAN interfaces must validate lengths, ranges and state before acting. The
draft CAN protocol limits remote commands to STOP; START and fault clearing remain
UART-only until an authorization decision is made ([docs/can_protocol.md](docs/can_protocol.md)).
