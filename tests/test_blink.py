#!/usr/bin/env python3
"""Host test for dontfall-app's --led blink mode: a temp directory stands
in for /sys/class/leds/<name>, and a pty can stand in for /dev/ttyRPMSG0.

Usage: test_blink.py <path-to-dontfall-app>
"""
import os
import pty
import select
import signal
import subprocess
import sys
import tempfile
import time

APP = sys.argv[1]


def read_line(fd, timeout):
    buf = b""
    deadline = time.monotonic() + timeout
    while not buf.endswith(b"\n"):
        left = deadline - time.monotonic()
        if left <= 0 or not select.select([fd], [], [], left)[0]:
            break
        buf += os.read(fd, 256)
    return buf


def make_led():
    tmp = tempfile.TemporaryDirectory()
    led = tmp.name
    with open(os.path.join(led, "trigger"), "w") as f:
        f.write("none [heartbeat] timer\n")
    with open(os.path.join(led, "brightness"), "w") as f:
        f.write("0\n")
    return tmp, led


def start(args):
    return subprocess.Popen([APP] + args, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True)


def read(led, name):
    with open(os.path.join(led, name)) as f:
        return f.read().strip()


def sample_brightness(led, seconds, every):
    seen = set()
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        seen.add(read(led, "brightness"))
        time.sleep(every)
    return seen


def test_blink_without_m4():
    # hello fails (no device) but blinking goes on; SIGTERM restores the LED
    tmp, led = make_led()
    proc = start(["--led", led, "/nonexistent/tty"])
    try:
        seen = sample_brightness(led, seconds=2.5, every=0.05)
        assert proc.poll() is None, "app exited instead of blinking"
        assert read(led, "trigger") == "none", "trigger not taken over"
        assert {"0", "1"} <= seen, f"brightness values seen {seen}"
        t = time.monotonic()
        proc.send_signal(signal.SIGTERM)
        out, err = proc.communicate(timeout=2)
        assert proc.returncode == 0, f"exit {proc.returncode}, stderr {err!r}"
        assert time.monotonic() - t < 0.5, "SIGTERM waited for the next tick"
        assert read(led, "brightness") == "0"
        assert read(led, "trigger") == "heartbeat"
        assert "cannot open /nonexistent/tty" in err
    finally:
        proc.kill()
        proc.communicate()
        tmp.cleanup()


def test_blink_with_hello():
    # pty plays the M4 as in test_rpmsg_hello.py
    tmp, led = make_led()
    master, slave = pty.openpty()
    proc = start(["--led", led, os.ttyname(slave)])
    try:
        assert read_line(master, 3) == b"hello from A7\n"
        os.write(master, b"hello from M4: hello from A7\n")
        time.sleep(1.5)
        assert proc.poll() is None, "app exited after the hello"
        proc.send_signal(signal.SIGTERM)
        out, err = proc.communicate(timeout=2)
        assert proc.returncode == 0, f"exit {proc.returncode}, stderr {err!r}"
        assert "hello from M4: hello from A7" in out, f"stdout {out!r}"
    finally:
        proc.kill()
        proc.communicate()
        os.close(master)
        os.close(slave)
        tmp.cleanup()


def test_led_missing():
    proc = start(["--led", "/nonexistent/led", "/nonexistent/tty"])
    out, err = proc.communicate(timeout=5)
    assert proc.returncode == 1
    assert "cannot read /nonexistent/led/trigger" in err


if __name__ == "__main__":
    failed = 0
    for test in (test_blink_without_m4, test_blink_with_hello,
                 test_led_missing):
        try:
            test()
            print(f"PASS {test.__name__}")
        except Exception as e:  # report every case, not just the first
            failed += 1
            print(f"FAIL {test.__name__}: {e}")
    sys.exit(1 if failed else 0)
