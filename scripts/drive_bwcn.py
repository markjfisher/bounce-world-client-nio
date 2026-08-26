#!/usr/bin/env python3
"""Drive the Bouncy World Amiga client under Amiberry from the host.

Checked-in exemplar for controlling an interactive Amiberry Workbench
session through the workspace `amiga_emulator` toolkit. Launches a shell,
starts `NIO:bwcn.amiga`, fills the connect screen, and steps through the
shapes preview into the main loop, capturing screenshots as evidence.

Prerequisites:
  - Amiberry running with IPC socket support, e.g.:
      ./scripts/build.sh amiga-workbench --profile wb32-a1200 -- --external-nio --tcp
  - FujiNet NIO reachable over TCP (the client's server URL, default
    192.168.1.101:9003, must point at your Bouncy World server).
  - `NIO:` development share containing a current `bwcn.amiga`
    (refresh links in build/amiga-share after rebuilding).

Usage:
  python3 scripts/drive_bwcn.py                        # full run
  python3 scripts/drive_bwcn.py --stage connect        # up to the connect screen
  python3 scripts/drive_bwcn.py --stage run            # spaces into the loop
  python3 scripts/drive_bwcn.py --server 10.0.0.5:9003 --name ami2

See docs/amiga/amiberry-control-examples.md (workspace) for background and
gotchas: modifier chords, requester close keys, and the two delay types.
"""
from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

WORKSPACE_TOOLS = Path(__file__).resolve().parents[3] / "tools"
sys.path.insert(0, str(WORKSPACE_TOOLS))

from amiga_emulator import ipc  # noqa: E402
from amiga_emulator.keyboard import Keyboard  # noqa: E402


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Drive the Bouncy World Amiga client via Amiberry IPC.")
    parser.add_argument("--socket",
                        help="Amiberry IPC socket (default: autodetect)")
    parser.add_argument("--delay", type=float, default=0.02,
                        help="per-keystroke hold delay in seconds")
    parser.add_argument("--server", default="192.168.1.101:9003",
                        help="Bouncy World server URL for the connect screen")
    parser.add_argument("--name", default="ami",
                        help="client name for the connect screen")
    parser.add_argument("--shots", default="/tmp/kilo/bwcn-drive",
                        help="screenshot output directory")
    parser.add_argument("--stage", default="all",
                        choices=["all", "connect", "run"],
                        help="connect: shell+launch+fields; run: spaces")
    return parser.parse_args(argv)


def wait_for_workbench(socket: Path, timeout: float = 60.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            ipc.request(socket, "GET_STATUS")
            time.sleep(5)  # let Workbench settle after boot
            return
        except Exception:
            time.sleep(2)
    raise SystemExit("Amiberry IPC never became ready")


def main() -> int:
    args = parse_args()
    socket = ipc.find_socket(args.socket)
    keyboard = Keyboard(socket=socket, delay=args.delay)
    shots = Path(args.shots)
    shots.mkdir(parents=True, exist_ok=True)

    def shot(name: str, settle: float = 1.0) -> None:
        keyboard.screenshot(shots / name, settle=settle)

    wait_for_workbench(socket)

    if args.stage in ("all", "connect"):
        # Open a fresh shell and launch the client from the NIO: share.
        # The {delay:0.3} covers the Execute-dialog focus change; per-
        # keystroke --delay does not (see the workspace doc's gotchas).
        keyboard.type_text("{ramiga+e}{delay:0.3}newshell{return}")
        time.sleep(2.0)
        shot("10-shell.png")
        keyboard.type_text("nio:bwcn.amiga{return}")
        time.sleep(3.0)  # client startup -> connect screen
        shot("20-connect.png")
        keyboard.type_text("s")
        time.sleep(0.3)
        keyboard.type_text(args.server + "{return}")
        time.sleep(0.5)
        keyboard.type_text("n")
        time.sleep(0.3)
        keyboard.type_text(args.name + "{return}")
        time.sleep(0.5)
        shot("30-fields.png")

    if args.stage in ("all", "run"):
        keyboard.type_text("{space}")  # shapes preview
        time.sleep(2.5)
        shot("40-preview.png")
        keyboard.type_text("{space}")  # into the main loop
        time.sleep(5)
        shot("50-loop.png")
        time.sleep(10)
        shot("60-loop-later.png")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
