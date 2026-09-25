import contextlib
import io
import itertools
import json
import sqlite3
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from baccable_lab.capture import capture
from baccable_lab.cli import _export_basic, _session_info, main

FRAME = bytes([0xA1, 1, 0, 0, 0x34, 0x12, 0, 0, 0xAA] + [0] * 7)


class CaptureTests(unittest.TestCase):
    def test_all_bus_selections_and_empty_bus(self):
        all_roles = ('C1', 'C2', 'BH')
        selections = [roles for count in range(1, 4)
                      for roles in itertools.combinations(all_roles, count)]
        for roles, has_frames in [(roles, True) for roles in selections] + [(('BH',), False)]:
            with self.subTest(roles=roles, has_frames=has_frames), tempfile.TemporaryDirectory() as temp:
                opened = []

                class Reader:
                    error = None

                    def __init__(self, role, device, output, stop):
                        self.role, self.device, self.output = role, device, output

                    def start(self):
                        opened.append((self.role, self.device))
                        if has_frames:
                            self.output.put((self.role, time.monotonic_ns(), FRAME))

                    def join(self, timeout):
                        pass

                ports = {role: f'/dev/test-{role}' for role in roles}
                with patch('baccable_lab.capture._Reader', Reader), \
                        patch('baccable_lab.capture._Keyboard') as keyboard, \
                        contextlib.redirect_stdout(io.StringIO()):
                    keyboard.return_value.read.side_effect = [None] * len(roles) + ['q']
                    self.assertEqual(capture(ports, Path(temp), ['capture']), 0)
                self.assertEqual(dict(opened), ports)
                directory, = Path(temp).iterdir()
                self.assertEqual({p.stem for p in directory.glob('*.bin')}, set(roles))
                for role in roles:
                    self.assertEqual((directory / f'{role}.bin').read_bytes(), FRAME if has_frames else b'')
                manifest = json.loads((directory / 'manifest.json').read_text())
                summary = json.loads((directory / 'summary.json').read_text())
                self.assertEqual(manifest['roles'], ports)
                self.assertEqual(set(summary['roles']), set(roles))
                with sqlite3.connect(directory / 'session.sqlite3') as db:
                    self.assertEqual(dict(db.execute('SELECT role, count(*) FROM can_frames GROUP BY role')),
                                     dict.fromkeys(roles, 1) if has_frames else {})
                with contextlib.redirect_stdout(io.StringIO()) as info:
                    self.assertEqual(_session_info(directory), 0)
                for role in all_roles:
                    self.assertEqual(f'  {role}:' in info.getvalue(), role in roles)
                with contextlib.redirect_stdout(io.StringIO()) as exported:
                    self.assertEqual(_export_basic(directory, None), 0)
                self.assertEqual(len(exported.getvalue().splitlines()), (len(roles) if has_frames else 0) + 1)

    def test_invalid_selections_do_not_create_sessions(self):
        for ports in ({}, {'OTHER': '/dev/a'}, {'C1': ''}, {'C1': '/dev/a', 'BH': '/dev/a'}):
            with self.subTest(ports=ports), tempfile.TemporaryDirectory() as temp:
                root = Path(temp) / 'sessions'
                with self.assertRaises(ValueError):
                    capture(ports, root, [])
                self.assertFalse(root.exists())

    def test_cli_requires_a_port(self):
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as result:
            main(['capture'])
        self.assertEqual(result.exception.code, 2)


if __name__ == '__main__':
    unittest.main()
