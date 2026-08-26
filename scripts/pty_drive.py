#!/usr/bin/env python3
"""Drive the linux Bouncy World client under a pty and capture output.

Host-side reference soak: runs build/bwcn.linux against the FujiNet PTY
instance with scripted keystrokes, capturing all terminal output. Useful
for diffing Amiga behaviour against a controllable target.
"""
import os
import pty
import select
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
CLIENT = REPO / "build" / "bwcn.linux"
OUT = Path("/tmp/kilo/linux-client.log")


def main() -> int:
    master, slave = pty.openpty()
    env = dict(os.environ, FN_PORT="/tmp/fujinet-nio-pty", TERM="vt100")
    pid = os.fork()
    if pid == 0:
        os.setsid()
        os.dup2(slave, 0)
        os.dup2(slave, 1)
        os.dup2(slave, 2)
        os.close(master)
        os.close(slave)
        os.chdir(REPO)
        os.execve(CLIENT, [CLIENT], env)
    os.close(slave)

    log = OUT.open("wb")
    start = time.time()

    def send(data: bytes) -> None:
        os.write(master, data)

    def pump(duration: float) -> None:
        end = time.time() + duration
        while time.time() < end:
            ready, _, _ = select.select([master], [], [], 0.2)
            if ready:
                try:
                    log.write(os.read(master, 4096))
                    log.flush()
                except OSError:
                    return

    try:
        pump(3)          # boot / getinfo screen
        send(b" ")       # through shapes preview
        pump(60)         # soak in the main loop; Ctrl-C/q to exit early
    finally:
        send(b"q")
        pump(2)
        log.close()
        print(f"log: {OUT} ({time.time() - start:.0f}s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
