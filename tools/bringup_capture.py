#!/usr/bin/env python3
"""Capture and log Phase 0 bring-up output from the ESP32 over a serial port.

Reads the log lines printed by firmware/bringup (blink / ADC / INA226 /
DS18B20 tests), parses the measurements, and writes a JSON-lines record with
run metadata, every parsed sample, and a summary.

A pass/fail verdict is produced ONLY when the operator supplies an
independent reference value and a tolerance (--reference/--tolerance), e.g.
a multimeter reading. Without them the summary says "not_evaluated". The
tool never invents thresholds and never fabricates samples.

Examples:
  python3 tools/bringup_capture.py --list-ports
  python3 tools/bringup_capture.py --test ds18b20 --port /dev/ttyUSB0 \\
      --duration 20 --reference 23.5 --tolerance 2.0 --out run_ds18b20.jsonl
  python3 tools/bringup_capture.py --test adc --replay saved_monitor.log --out replay.jsonl

Exit codes: 0 ok / not evaluated, 1 reference check failed or errors seen
with a check requested, 2 usage or port error, 3 no matching samples,
4 capture ended early because the serial port failed (partial data is kept).
"""
import argparse
import json
import os
import re
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone

TOOL_VERSION = "0.1.0"

ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
LOG_RE = re.compile(r"^([EWIDV]) \((\d+)\) ([\w\-]+): (.*)$")
NUM = r"(-?\d+(?:\.\d+)?)"

# test name -> (log tag, primary quantity key/unit, [(regex, extractor)])
TESTS = {
    "adc": {
        "tag": "test_adc_read",
        "primary": ("divider_input_v", "V"),
        "patterns": [
            (re.compile(r"raw=(\d+)\s+adc_mv=(\d+)\s+divider_input\s*[=≈]\s*" + NUM + r"\s*V"),
             lambda m: {"raw": int(m[1]), "adc_mv": int(m[2]), "divider_input_v": float(m[3])}),
            (re.compile(r"raw=(\d+) \(uncalibrated\)"), lambda m: {"raw": int(m[1])}),
        ],
    },
    "ina226": {
        "tag": "test_i2c_ina226",
        "primary": ("bus_voltage_v", "V"),
        "patterns": [
            (re.compile(r"bus_voltage\s*[=≈]\s*" + NUM + r" V \(raw=0x([0-9A-Fa-f]+)\)"),
             lambda m: {"bus_voltage_v": float(m[1]), "raw": int(m[2], 16)}),
            (re.compile(r"manufacturer ID = 0x([0-9A-Fa-f]{4})"),
             lambda m: {"manufacturer_id": int(m[1], 16)}),
            (re.compile(r"die ID = 0x([0-9A-Fa-f]{4})"), lambda m: {"die_id": int(m[1], 16)}),
        ],
    },
    "ds18b20": {
        "tag": "test_ds18b20",
        "primary": ("temperature_c", "C"),
        "patterns": [
            (re.compile(r"temperature\s*[=≈]\s*" + NUM + r"\s*°?\s*C"),
             lambda m: {"temperature_c": float(m[1])}),
        ],
    },
    "blink": {"tag": "test_blink", "primary": None, "patterns": []},
}


def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def parse_line(line, test):
    """Return a record dict for one console line, or None if it is not a log line."""
    line = ANSI_RE.sub("", line).strip()
    if test == "blink" and line == "pong":
        return {"level": "-", "tag": "console", "values": {"pong": True}, "raw_line": line}
    m = LOG_RE.match(line)
    if not m:
        return None
    level, device_ms, tag, msg = m.group(1), int(m.group(2)), m.group(3), m.group(4)
    rec = {"level": level, "device_ms": device_ms, "tag": tag, "message": msg,
           "values": {}, "raw_line": line}
    if tag == TESTS[test]["tag"]:
        for regex, extract in TESTS[test]["patterns"]:
            mm = regex.search(msg)
            if mm:
                rec["values"] = extract(mm)
                break
    return rec


def git_info():
    here = os.path.dirname(os.path.abspath(__file__))
    try:
        rev = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=here,
                             capture_output=True, text=True, timeout=5)
        dirty = subprocess.run(["git", "status", "--porcelain"], cwd=here,
                               capture_output=True, text=True, timeout=5)
        if rev.returncode == 0:
            return {"commit": rev.stdout.strip(), "dirty": bool(dirty.stdout.strip())}
    except (OSError, subprocess.SubprocessError):
        pass
    return {"commit": None, "dirty": None}


def list_ports():
    try:
        from serial.tools import list_ports as lp
    except ImportError:
        sys.exit("pyserial is required to list ports: pip install -r tools/requirements.txt")
    return [(p.device, p.description) for p in lp.comports()]


def open_serial(port, baud, reset):
    try:
        import serial
    except ImportError:
        sys.exit("pyserial is required for live capture: pip install -r tools/requirements.txt")
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = port, baud, 0.2
    if not reset:  # keep DTR/RTS low so the board is not reset by opening the port
        ser.dtr = False
        ser.rts = False
    try:
        ser.open()
    except (serial.SerialException, OSError) as exc:
        sys.exit(f"cannot open serial port {port}: {exc}")
    return ser


def serial_lines(ser, duration_s, send_ping):
    """Yield decoded lines until the deadline. Stops cleanly on a serial error."""
    import serial
    deadline = time.monotonic() + duration_s
    buf = b""
    next_ping = 0.0  # resend periodically: the board may still be booting after a reset
    while time.monotonic() < deadline:
        try:
            if send_ping and time.monotonic() >= next_ping:
                ser.write(b"ping\n")
                next_ping = time.monotonic() + 2.0
            chunk = ser.read(256)
        except (serial.SerialException, OSError) as exc:
            yield ("__error__", f"serial read failed: {exc}")
            return
        if not chunk:
            continue
        buf += chunk
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            yield ("line", raw.decode("utf-8", errors="replace"))


def replay_lines(path):
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            yield ("line", raw)


def summarize(samples, test, errors, warnings, terminated, reference, tolerance):
    primary = TESTS[test]["primary"]
    summary = {"type": "summary", "test": test, "n_samples": len(samples),
               "n_error_lines": errors, "n_warning_lines": warnings,
               "terminated": terminated}
    values = []
    if primary:
        key, unit = primary
        values = [s["values"][key] for s in samples if key in s["values"]]
        summary["primary_quantity"] = {"name": key, "unit": unit, "n": len(values)}
        if values:
            summary["primary_quantity"].update(
                mean=statistics.fmean(values), min=min(values), max=max(values))
    check = {"performed": False, "result": "not_evaluated"}
    if test == "blink":
        got_pong = any(s["values"].get("pong") for s in samples)
        check = {"performed": True, "criterion": "'pong' received after 'ping'",
                 "result": "pass" if got_pong else "fail"}
    elif reference is not None:
        if len(values) < 3:
            check = {"performed": True, "result": "fail",
                     "criterion": "needs >= 3 samples", "reference": reference,
                     "tolerance": tolerance}
        else:
            mean = statistics.fmean(values)
            ok = abs(mean - reference) <= tolerance and errors == 0
            check = {"performed": True, "reference": reference, "tolerance": tolerance,
                     "criterion": "|mean - reference| <= tolerance and no error lines",
                     "mean": mean, "result": "pass" if ok else "fail"}
    summary["check"] = check
    return summary


def build_parser():
    p = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                formatter_class=argparse.RawDescriptionHelpFormatter,
                                epilog=__doc__.split("\n\n", 1)[1])
    p.add_argument("--list-ports", action="store_true", help="list serial ports and exit")
    p.add_argument("--test", choices=sorted(TESTS), help="which bring-up test is running")
    p.add_argument("--port", help="serial device; if omitted and exactly one port exists it is used")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=float, default=10.0, help="capture seconds (default 10)")
    p.add_argument("--replay", metavar="FILE", help="parse a saved monitor log instead of a port")
    p.add_argument("--out", help="JSON-lines output file (required unless --list-ports)")
    p.add_argument("--reference", type=float,
                   help="independent reference value for the primary quantity (e.g. multimeter)")
    p.add_argument("--tolerance", type=float, help="allowed |mean - reference|; required with --reference")
    p.add_argument("--note", default="", help="free-text operator note stored in the log")
    p.add_argument("--no-reset", action="store_true", help="do not reset the board when opening the port")
    return p


def main(argv=None):
    args = build_parser().parse_args(argv)

    if args.list_ports:
        for dev, desc in list_ports():
            print(f"{dev}\t{desc}")
        return 0
    if not args.test or not args.out:
        print("error: --test and --out are required", file=sys.stderr)
        return 2
    if args.reference is not None and (args.tolerance is None or args.tolerance < 0):
        print("error: --reference needs a non-negative --tolerance", file=sys.stderr)
        return 2
    if args.reference is not None and TESTS[args.test]["primary"] is None:
        print("error: --reference is not applicable to this test", file=sys.stderr)
        return 2
    if args.duration <= 0:
        print("error: --duration must be > 0", file=sys.stderr)
        return 2

    ser = None
    port = args.port
    if args.replay:
        if not os.path.isfile(args.replay):
            print(f"error: replay file not found: {args.replay}", file=sys.stderr)
            return 2
        source = replay_lines(args.replay)
    else:
        if not port:
            ports = list_ports()
            if len(ports) != 1:
                print("error: pass --port; found: " + (", ".join(d for d, _ in ports) or "none"),
                      file=sys.stderr)
                return 2
            port = ports[0][0]
        ser = open_serial(port, args.baud, reset=not args.no_reset)
        source = serial_lines(ser, args.duration, send_ping=(args.test == "blink"))

    meta = {"type": "meta", "tool": "bringup_capture.py", "tool_version": TOOL_VERSION,
            "started_utc": utc_now(), "test": args.test, "mode": "replay" if args.replay else "serial",
            "port": port, "baud": args.baud, "duration_s": args.duration,
            "replay_file": args.replay, "reference": args.reference, "tolerance": args.tolerance,
            "operator_note": args.note, "python": sys.version.split()[0], "repo": git_info()}
    try:
        import serial
        meta["pyserial"] = serial.VERSION
    except ImportError:
        meta["pyserial"] = None

    samples, errors, warnings, terminated = [], 0, 0, "end_of_input" if args.replay else "duration_elapsed"
    try:
        with open(args.out, "w", encoding="utf-8") as out:
            out.write(json.dumps(meta) + "\n")
            for kind, payload in source:
                if kind == "__error__":
                    terminated = "serial_error"
                    out.write(json.dumps({"type": "event", "host_time_utc": utc_now(),
                                          "message": payload}) + "\n")
                    break
                rec = parse_line(payload, args.test)
                if rec is None:
                    continue
                if rec["level"] == "E":
                    errors += 1
                elif rec["level"] == "W":
                    warnings += 1
                if rec["values"]:
                    samples.append(rec)
                if rec["values"] or rec["level"] in ("E", "W"):
                    rec = dict(rec, type="sample" if rec["values"] else "log",
                               host_time_utc=utc_now())
                    out.write(json.dumps(rec, ensure_ascii=False) + "\n")
            summary = summarize(samples, args.test, errors, warnings, terminated,
                                args.reference, args.tolerance)
            out.write(json.dumps(summary) + "\n")
    except OSError as exc:
        print(f"error: cannot write {args.out}: {exc}", file=sys.stderr)
        return 2
    finally:
        if ser is not None:
            ser.close()

    print(json.dumps({k: summary[k] for k in ("test", "n_samples", "n_error_lines", "terminated", "check")}))
    if terminated == "serial_error":
        print("warning: serial port failed mid-capture; log is incomplete", file=sys.stderr)
        return 4
    if not samples:
        print("warning: no matching samples captured", file=sys.stderr)
        return 3
    return 1 if summary["check"]["result"] == "fail" else 0


if __name__ == "__main__":
    sys.exit(main())
