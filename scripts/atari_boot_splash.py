#!/usr/bin/env python3
"""Build the Atari loading screen and prefix its INIT segment to a XEX."""
import argparse
from pathlib import Path
import struct

ROWS = [
    "  .            *               .      ",
    "         .             .            * ",
    "    *          /\\           .        ",
    "              /  \\     *              ",
    "    .        / /\\ \\             .    ",
    "             \\ \\/ /       /--\\       ",
    "  *           \\__/       /    \\      ",
    "                         \\    /   *  ",
    "    .      B O U N C Y    \\__/        ",
    "                                      ",
    "          W O R L D        .          ",
    "   *                              .   ",
    "       /--\\                 *        ",
    "      /    \\    .                    ",
    "      \\    /          /\\           . ",
    "       \\__/          /  \\            ",
    " .                    \\  /     *      ",
    "                       \\/             ",
    "         *                     .      ",
    "          LOADING THE WORLD...        ",
    "                                      ",
    "  .        *          .            *  ",
    "                                      ",
    "                                      ",
]

def screen_code(c):
    n = ord(c)
    return n - 32 if 32 <= n < 96 else n - 32 if 96 <= n < 128 else n

def main():
    p = argparse.ArgumentParser()
    p.add_argument('mode', choices=['screen', 'xex'])
    p.add_argument('output', type=Path)
    p.add_argument('inputs', nargs='*', type=Path)
    a = p.parse_args()
    if a.mode == 'screen':
        if len(ROWS) != 24 or any(len(row) > 40 for row in ROWS):
            raise ValueError('splash must fit 40x24')
        a.output.write_bytes(bytes(screen_code(c) for row in ROWS for c in row.ljust(40)))
        return
    loader, app = (p.read_bytes() for p in a.inputs)
    if not loader or len(loader) > 0x800:
        raise ValueError('splash loader exceeds reserved area')
    # Reject app load chunks that overwrite the standard OS text screen.
    pos = 0
    while pos < len(app):
        start, = struct.unpack_from('<H', app, pos); pos += 2
        if start == 0xffff:
            continue
        end, = struct.unpack_from('<H', app, pos); pos += 2
        if end < start or pos + end - start + 1 > len(app):
            raise ValueError('invalid application XEX')
        if start <= 0x9fff and end >= 0x9800:
            raise ValueError('application load overlaps the splash screen area')
        pos += end - start + 1
    segment = lambda lo, hi: struct.pack('<HH', lo, hi)
    a.output.write_bytes(b'\xff\xff' + segment(0x8000, 0x8000 + len(loader) - 1)
                         + loader + segment(0x02e2, 0x02e3) + b'\x00\x80' + app)

if __name__ == '__main__':
    main()
