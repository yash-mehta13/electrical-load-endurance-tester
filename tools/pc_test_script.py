"""
PC-side test automation for the Load Endurance & Fault-Diagnostic Test Controller.

Placeholder — built out in Phase 7, once the firmware's UART command set
(SET/START/STOP/STATUS) exists (Phase 3 onward). Planned shape:

    - Open the serial port (pyserial)
    - Send SET commands for the test profile, then START
    - Wait for COMPLETE or a LATCHED_OFF fault line
    - Parse the resulting log into a structured summary (cycles completed,
      fault counts by class, min/max/avg current & temperature)

Not implemented yet.
"""

if __name__ == "__main__":
    raise NotImplementedError("Phase 7 — not yet implemented")
