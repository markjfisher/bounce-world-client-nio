"""Verify the splash XEX prefix and protect its screen from load segments."""
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SCRIPT = REPO / 'scripts/atari_boot_splash.py'


class SplashTest(unittest.TestCase):
    def test_screen_and_xex(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            screen, loader, app, output = [tmp / n for n in ('screen', 'loader', 'app', 'out')]
            subprocess.run([sys.executable, SCRIPT, 'screen', screen], check=True)
            self.assertEqual(len(screen.read_bytes()), 960)
            loader.write_bytes(b'\xea\x60')
            app.write_bytes(b'\xff\xff' + struct.pack('<HH', 0x1800, 0x1800) + b'\x60')
            subprocess.run([sys.executable, SCRIPT, 'xex', output, loader, app], check=True)
            data = output.read_bytes()
            self.assertEqual(data[:8], b'\xff\xff\x00\x80\x01\x80\xea\x60')
            self.assertEqual(data[8:14], b'\xe2\x02\xe3\x02\x00\x80')
            self.assertEqual(data[14:], app.read_bytes())
            app.write_bytes(b'\xff\xff' + struct.pack('<HH', 0x9800, 0x9800) + b'\x60')
            rejected = subprocess.run([sys.executable, SCRIPT, 'xex', output, loader, app],
                                      capture_output=True)
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn(b'overlaps', rejected.stderr)
