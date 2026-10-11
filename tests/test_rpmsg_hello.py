#!/usr/bin/env python3
"""Host test for dontfall-app's OpenAMP hello: a pty stands in for
/dev/ttyRPMSG0 and this script plays the M4.

Usage: test_rpmsg_hello.py <path-to-dontfall-app>
"""
import os
import pty
import select
import subprocess
import sys
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


def run_app(dev):
    return subprocess.Popen([APP, dev], stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True)


def test_reply():
    master, slave = pty.openpty()
    proc = run_app(os.ttyname(slave))
    got = read_line(master, 3)
    assert got == b"hello from A7\n", f"app sent {got!r}"
    os.write(master, b"hello from M4: hello from A7\n")
    out, err = proc.communicate(timeout=5)
    assert proc.returncode == 0, f"exit {proc.returncode}, stderr {err!r}"
    assert "hello from M4: hello from A7" in out, f"stdout {out!r}"


def test_timeout():
    master, slave = pty.openpty()
    start = time.monotonic()
    proc = run_app(os.ttyname(slave))
    out, err = proc.communicate(timeout=6)
    assert proc.returncode == 2, f"exit {proc.returncode}, stderr {err!r}"
    assert time.monotonic() - start < 4, "timeout took too long"
    assert "no reply from M4" in err, f"stderr {err!r}"


def test_missing_device():
    proc = run_app("/nonexistent/tty")
    out, err = proc.communicate(timeout=5)
    assert proc.returncode == 1, f"exit {proc.returncode}, stderr {err!r}"
    assert "cannot open /nonexistent/tty" in err, f"stderr {err!r}"


if __name__ == "__main__":
    failed = 0
    for test in (test_reply, test_timeout, test_missing_device):
        try:
            test()
            print(f"PASS {test.__name__}")
        except Exception as e:  # report every case, not just the first
            failed += 1
            print(f"FAIL {test.__name__}: {e}")
    sys.exit(1 if failed else 0)
