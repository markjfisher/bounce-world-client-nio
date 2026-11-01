"""Headless Atari boot and quit-to-title integration test.

Run from the repository root after `make disk-atari`:
    python3 -m unittest discover -s tests/atari -p test_presentation_boot.py -v
The workspace's NetSIO firmware, hub and AltirraSDL builds must be available.
"""
import os
import socket
import struct
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
WORKSPACE = REPO.parents[1]
sys.path.insert(0, str(WORKSPACE / 'repos/nio-config/scripts'))
import atari_shot_netsio


class PresentationBootTest(unittest.TestCase):
    def test_boot_play_and_return_to_title(self):
        server = socket.socket()
        server.bind(('127.0.0.1', 0))
        server.listen()
        port = server.getsockname()[1]
        commands = []

        def serve():
            while True:
                try:
                    conn, _ = server.accept()
                except OSError:
                    return
                try:
                    with conn, conn.makefile('rb') as incoming:
                        while line := incoming.readline():
                            cmd = line.decode().strip()
                            commands.append(cmd)
                            if cmd.startswith('x-add-client'):
                                payload = b'\x01'
                            elif cmd == 'x-ws':
                                payload = bytes([40, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0])
                            elif cmd == 'x-who':
                                payload = b'tester  '
                            elif cmd.startswith('x-w '):
                                payload = b'\x01\x00\x00'
                            else:
                                payload = b'\x00'
                            conn.sendall(struct.pack('<H', len(payload) + 2) + payload)
                except OSError:
                    pass

        threading.Thread(target=serve, daemon=True).start()
        with tempfile.TemporaryDirectory(prefix='bwc-boot-') as tmp:
            def run(view):
                import atari_ui
                screen_dir = os.environ.get('BWC_SCREEN_DIR')
                if screen_dir:
                    Path(screen_dir).mkdir(parents=True, exist_ok=True)
                def screenshot(name):
                    if screen_dir:
                        view.screenshot(Path(screen_dir) / name)
                def wait(text, timeout=90):
                    until = time.monotonic() + timeout
                    while time.monotonic() < until:
                        rows = atari_ui.read_os_screen(view.bridge)
                        if any(text in row for row in rows):
                            return
                        time.sleep(.2)
                    self.fail(f'{text!r} not on screen:\n' + '\n'.join(rows))

                wait('Press a key to continue')
                time.sleep(1)
                screenshot('title.png')
                view.key('S')
                view.type(f'127.0.0.1:{port}')
                view.key('RETURN')
                time.sleep(2)
                view.key('N')
                view.type('tester')
                view.key('RETURN')
                time.sleep(2)
                view.key('SPACE')
                wait('BOUNCY WORLD / OBJECTS')
                time.sleep(1)
                screenshot('shapes.png')
                view.key('SPACE')
                until = time.monotonic() + 30
                while not any(c.startswith('x-w ') for c in commands):
                    self.assertLess(time.monotonic(), until, 'no simulation requests')
                    time.sleep(.2)
                view.key('Q')
                wait('SPACE CONTROL', 40)
                wait('Press a key to continue', 40)
                time.sleep(1)
                screenshot('restart.png')
                self.assertTrue(any(c.startswith('close ') for c in commands), commands)
                view.key('SPACE')
                wait('BOUNCY WORLD / OBJECTS')
                view.key('SPACE')
                until = time.monotonic() + 30
                while sum(c.startswith('x-add-client') for c in commands) < 2:
                    self.assertLess(time.monotonic(), until, 'second registration missing')
                    time.sleep(.2)
                view.key('Q')
                wait('SPACE CONTROL', 40)

            try:
                atari_shot_netsio.boot(REPO / 'disk-images/atari/bwcn.atr', run, out=tmp)
            finally:
                server.close()


if __name__ == '__main__':
    unittest.main()
