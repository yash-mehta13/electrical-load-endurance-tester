"""Tests for tools/bringup_capture.py.

IMPORTANT: every console line below is a SYNTHETIC format fixture written from
the format strings in firmware/bringup/main/*.c. None of it is output captured
from real hardware, and passing these tests says nothing about sensor accuracy.
"""
import json
import os
import sys
import tempfile
import threading
import time
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import bringup_capture as bc  # noqa: E402

try:
    import serial  # noqa: F401
    HAVE_SERIAL = True
except ImportError:
    HAVE_SERIAL = False

ADC_LINES = [
    "I (1000) test_adc_read: raw=1500  adc_mv=1210  divider_input=6.71V",
    "I (1500) test_adc_read: raw=1502  adc_mv=1211  divider_input=6.71V",
    "I (2000) test_adc_read: raw=1498  adc_mv=1209  divider_input=6.70V",
]
DS_LINES = [f"I ({1000 + i * 1000}) test_ds18b20: temperature = {t:.2f} C" for i, t in
            enumerate([23.40, 23.50, 23.60])]


def run_replay(test, lines, extra=None, tmp=None):
    d = tmp or tempfile.mkdtemp()
    log = os.path.join(d, "in.log")
    out = os.path.join(d, "out.jsonl")
    with open(log, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines) + "\n")
    rc = bc.main(["--test", test, "--replay", log, "--out", out] + (extra or []))
    with open(out, encoding="utf-8") as fh:
        recs = [json.loads(line) for line in fh]
    return rc, recs


class ParseTests(unittest.TestCase):
    def test_adc_line(self):
        r = bc.parse_line(ADC_LINES[0], "adc")
        self.assertEqual(r["values"], {"raw": 1500, "adc_mv": 1210, "divider_input_v": 6.71})
        self.assertEqual(r["device_ms"], 1000)

    def test_ansi_colour_codes_are_stripped(self):
        r = bc.parse_line("\x1b[0;32m" + DS_LINES[0] + "\x1b[0m", "ds18b20")
        self.assertEqual(r["values"], {"temperature_c": 23.4})

    def test_legacy_approx_format_and_degree_sign(self):
        r = bc.parse_line("I (5) test_ds18b20: temperature ≈ 21.25 °C", "ds18b20")
        self.assertEqual(r["values"], {"temperature_c": 21.25})

    def test_ina226_lines(self):
        r = bc.parse_line("I (1) test_i2c_ina226: bus_voltage = 12.000 V (raw=0x2580)", "ina226")
        self.assertEqual(r["values"]["raw"], 0x2580)
        r = bc.parse_line("I (1) test_i2c_ina226: manufacturer ID = 0x5449 (expected 0x5449 - unconfirmed)", "ina226")
        self.assertEqual(r["values"], {"manufacturer_id": 0x5449})

    def test_negative_temperature(self):
        r = bc.parse_line("I (1) test_ds18b20: temperature = -5.50 C", "ds18b20")
        self.assertEqual(r["values"], {"temperature_c": -5.5})

    def test_garbage_and_boot_noise_ignored(self):
        for line in ["", "ets Jun  8 2016 00:22:57", "rst:0x1 (POWERON_RESET)", "\x00\xff garbage"]:
            self.assertIsNone(bc.parse_line(line, "adc"))

    def test_other_tag_has_no_values(self):
        r = bc.parse_line("I (1) bringup: Phase 0 bring-up starting", "adc")
        self.assertEqual(r["values"], {})

    def test_pong(self):
        self.assertTrue(bc.parse_line("pong\r", "blink")["values"]["pong"])


class ReplayTests(unittest.TestCase):
    def test_no_reference_is_not_evaluated(self):
        rc, recs = run_replay("adc", ADC_LINES)
        self.assertEqual(rc, 0)
        self.assertEqual(recs[0]["type"], "meta")
        summ = recs[-1]
        self.assertEqual(summ["n_samples"], 3)
        self.assertEqual(summ["check"]["result"], "not_evaluated")
        self.assertAlmostEqual(summ["primary_quantity"]["mean"], (6.71 + 6.71 + 6.70) / 3)

    def test_reference_pass_and_fail(self):
        rc, recs = run_replay("ds18b20", DS_LINES, ["--reference", "23.5", "--tolerance", "0.5"])
        self.assertEqual((rc, recs[-1]["check"]["result"]), (0, "pass"))
        rc, recs = run_replay("ds18b20", DS_LINES, ["--reference", "30", "--tolerance", "0.5"])
        self.assertEqual((rc, recs[-1]["check"]["result"]), (1, "fail"))

    def test_error_line_fails_requested_check(self):
        rc, recs = run_replay("ds18b20", DS_LINES + ["E (4000) test_ds18b20: read failed: ESP_ERR_INVALID_CRC"],
                              ["--reference", "23.5", "--tolerance", "1"])
        self.assertEqual(rc, 1)
        self.assertEqual(recs[-1]["n_error_lines"], 1)

    def test_too_few_samples_cannot_pass(self):
        rc, recs = run_replay("ds18b20", DS_LINES[:2], ["--reference", "23.5", "--tolerance", "5"])
        self.assertEqual((rc, recs[-1]["check"]["result"]), (1, "fail"))

    def test_no_samples_exit_code(self):
        rc, recs = run_replay("adc", ["ets Jun  8 2016", "rst:0x1 (POWERON_RESET)"])
        self.assertEqual(rc, 3)
        self.assertEqual(recs[-1]["n_samples"], 0)

    def test_usage_errors(self):
        self.assertEqual(bc.main(["--test", "adc"]), 2)  # no --out
        self.assertEqual(bc.main(["--test", "adc", "--out", "x", "--reference", "1"]), 2)  # no tolerance
        self.assertEqual(bc.main(["--test", "blink", "--out", "x", "--reference", "1", "--tolerance", "1"]), 2)
        self.assertEqual(bc.main(["--test", "adc", "--out", "x", "--replay", "/nonexistent/file"]), 2)
        self.assertEqual(bc.main(["--test", "adc", "--out", "x", "--duration", "0", "--replay", "/dev/null"]), 2)

    def test_unwritable_output(self):
        log = os.path.join(tempfile.mkdtemp(), "in.log")
        with open(log, "w", encoding="utf-8") as fh:
            fh.write(ADC_LINES[0] + "\n")
        self.assertEqual(bc.main(["--test", "adc", "--replay", log, "--out", "/nonexistent_dir/o.jsonl"]), 2)


@unittest.skipUnless(HAVE_SERIAL and hasattr(os, "openpty"), "pyserial / pty not available")
class SerialPathTests(unittest.TestCase):
    """Exercise the real pyserial code path against a pseudo-terminal (no hardware)."""

    def _run(self, feeder, test, duration, extra=None):
        master, slave = os.openpty()
        out = os.path.join(tempfile.mkdtemp(), "o.jsonl")
        t = threading.Thread(target=feeder, args=(master,))
        t.start()
        try:
            rc = bc.main(["--test", test, "--port", os.ttyname(slave), "--duration", str(duration),
                          "--no-reset", "--out", out] + (extra or []))
        finally:
            t.join()
            for fd in (master, slave):
                try:
                    os.close(fd)
                except OSError:
                    pass
        with open(out, encoding="utf-8") as fh:
            return rc, [json.loads(line) for line in fh]

    def test_live_capture_split_writes(self):
        def feeder(fd):
            time.sleep(0.2)
            data = ("\n".join(DS_LINES) + "\n").encode()
            for i in range(0, len(data), 7):  # arbitrary chunking
                os.write(fd, data[i:i + 7])
                time.sleep(0.01)
        rc, recs = self._run(feeder, "ds18b20", 1.5)
        self.assertEqual(rc, 0)
        self.assertEqual(recs[-1]["n_samples"], 3)
        self.assertEqual(recs[-1]["terminated"], "duration_elapsed")

    def test_blink_ping_pong(self):
        def feeder(fd):
            deadline = time.time() + 3
            while time.time() < deadline:
                try:
                    if b"ping" in os.read(fd, 64):
                        os.write(fd, b"pong\r\n")
                        return
                except OSError:
                    return
        rc, recs = self._run(feeder, "blink", 3)
        self.assertEqual((rc, recs[-1]["check"]["result"]), (0, "pass"))

    def test_device_disconnect_midway(self):
        def feeder(fd):
            time.sleep(0.2)
            os.write(fd, (DS_LINES[0] + "\n").encode())
            time.sleep(0.2)
            os.close(fd)  # simulate unplug
        master, slave = os.openpty()
        out = os.path.join(tempfile.mkdtemp(), "o.jsonl")
        t = threading.Thread(target=feeder, args=(master,))
        t.start()
        rc = bc.main(["--test", "ds18b20", "--port", os.ttyname(slave), "--duration", "3",
                      "--no-reset", "--out", out])
        t.join()
        try:
            os.close(slave)
        except OSError:
            pass
        with open(out, encoding="utf-8") as fh:
            recs = [json.loads(line) for line in fh]
        self.assertEqual(recs[-1]["terminated"], "serial_error")
        self.assertEqual(rc, 4)  # truncated run is flagged, but data captured so far is kept
        self.assertEqual(recs[-1]["n_samples"], 1)

    def test_missing_port_exits_cleanly(self):
        with self.assertRaises(SystemExit) as cm:
            bc.main(["--test", "adc", "--port", "/dev/does-not-exist", "--out", "x.jsonl"])
        self.assertIn("cannot open serial port", str(cm.exception))


if __name__ == "__main__":
    unittest.main()
