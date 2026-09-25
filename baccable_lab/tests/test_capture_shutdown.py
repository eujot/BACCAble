import contextlib
import io
import json
import queue
import sqlite3
import tempfile
import threading
import types
import unittest
from pathlib import Path
from unittest.mock import patch

from baccable_lab.capture import capture, _Reader
from baccable_lab.storage.raw_writer import RawWriters
from baccable_lab.storage.sqlite_store import SessionStore

FRAME = bytes([0xa1, 1, 0, 0, 0x23, 1, 0, 0, 0xaa] + [0] * 7)


class ShutdownTests(unittest.TestCase):
    def test_full_queue_stop_and_disconnect(self):
        for disconnect, interrupt in ((False, False), (False, True), (True, False)):
            with self.subTest(disconnect=disconnect, interrupt=interrupt), tempfile.TemporaryDirectory() as temp:
                second_read, closed = threading.Event(), threading.Event()
                class Serial:
                    reads = 0
                    def __init__(self, *args, **kwargs): pass
                    def __enter__(self): return self
                    def __exit__(self, *args): closed.set()
                    def read(self, size):
                        self.reads += 1
                        if self.reads == 2:
                            second_read.set()
                            if disconnect: raise OSError('disconnected')
                        if self.reads <= 2: return FRAME
                        closed.wait(.01)
                        return b''
                def quit_when_full():
                    self.assertTrue(second_read.wait(2))
                    if interrupt:
                        raise KeyboardInterrupt
                    return 'q'
                incoming = queue.Queue(maxsize=1)
                with patch.dict('sys.modules', {'serial': types.SimpleNamespace(Serial=Serial)}), patch('baccable_lab.capture.queue.Queue', return_value=incoming), patch('baccable_lab.capture._Keyboard') as keyboard, contextlib.redirect_stdout(io.StringIO()):
                    keyboard.return_value.read.side_effect = quit_when_full
                    self.assertEqual(capture({'C1': 'fake'}, Path(temp), []), 2 if disconnect else 0)
                self.assertTrue(closed.is_set())
                self.assertFalse(any(t.name == 'capture-C1' for t in threading.enumerate()))
                directory, = Path(temp).iterdir()
                expected = 1 if disconnect else 2
                self.assertEqual((directory / 'C1.bin').read_bytes(), FRAME * expected)
                summary = json.loads((directory / 'summary.json').read_text())
                with contextlib.closing(sqlite3.connect(directory / 'session.sqlite3')) as db:
                    self.assertEqual(db.execute('select count(*) from can_frames').fetchone()[0], expected)
                    self.assertEqual(db.execute('select status from sessions').fetchone()[0], summary['status'])
                self.assertEqual(summary['status'], 'failed' if disconnect else 'complete')

    def test_abort_unblocks_full_queue_and_closes_port(self):
        read, closed = threading.Event(), threading.Event()
        class Serial:
            def __init__(self, *args, **kwargs): pass
            def __enter__(self): return self
            def __exit__(self, *args): closed.set()
            def read(self, size): read.set(); return FRAME
        incoming = queue.Queue(maxsize=1)
        incoming.put(('occupied', 0, b''))
        stop, abort = threading.Event(), threading.Event()
        with patch.dict('sys.modules', {'serial': types.SimpleNamespace(Serial=Serial)}):
            reader = _Reader('C1', 'fake', incoming, stop, abort)
            reader.start()
            self.assertTrue(read.wait(2))
            stop.set(); abort.set()
            reader.join(2)
        self.assertFalse(reader.is_alive())
        self.assertTrue(closed.is_set())
        self.assertEqual(reader.discarded_bytes, len(FRAME))

    def test_partial_start_and_io_failures_release_resources(self):
        for failure in ('start', 'write', 'raw_close', 'commit', 'summary', 'keyboard'):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as temp:
                stores, writers, readers = [], [], []
                class Store(SessionStore):
                    def __init__(self, *args): super().__init__(*args); stores.append(self)
                    def commit(self):
                        if failure == 'commit': raise OSError('disk full')
                        super().commit()
                class Raw(RawWriters):
                    def __init__(self, *args): super().__init__(*args); writers.append(self)
                    def write(self, *args):
                        if failure == 'write': raise OSError('disk full')
                        return super().write(*args)
                    def close(self):
                        super().close()
                        if failure == 'raw_close': raise OSError('flush failed')
                class Reader:
                    error = None
                    discarded_bytes = 0
                    def __init__(self, role, device, output, stop, abort):
                        self.role, self.output, self.joined = role, output, False
                    def start(self):
                        if failure == 'start' and self.role == 'C2': raise RuntimeError('thread start failed')
                        readers.append(self)
                        self.output.put((self.role, 0, FRAME))
                    def is_alive(self): return False
                    def join(self, timeout): self.joined = True
                original_write = Path.write_text
                def write_text(path, *args, **kwargs):
                    if failure == 'summary' and path.name == 'summary.json': raise OSError('disk full')
                    return original_write(path, *args, **kwargs)
                with patch('baccable_lab.capture.SessionStore', Store), patch('baccable_lab.capture.RawWriters', Raw), patch('baccable_lab.capture._Reader', Reader), patch('baccable_lab.capture._Keyboard') as keyboard, patch.object(Path, 'write_text', write_text), contextlib.redirect_stdout(io.StringIO()):
                    keyboard.return_value.read.side_effect = [None, 'q']
                    if failure == 'keyboard': keyboard.side_effect = OSError('terminal failed')
                    self.assertEqual(capture({'C1': 'one', 'C2': 'two'}, Path(temp), []), 2)
                    if failure != 'keyboard': keyboard.return_value.close.assert_called_once()
                self.assertTrue(all(r.joined for r in readers))
                self.assertTrue(all(f.closed for w in writers for f in w.files.values()))
                for store in stores:
                    with self.assertRaises(sqlite3.ProgrammingError): store.database.execute('select 1')

    def test_partial_raw_open_closes_previous_files(self):
        with tempfile.TemporaryDirectory() as temp:
            original, opened = Path.open, []
            def open_file(path, *args, **kwargs):
                if path.name == 'C2.bin': raise OSError('open failed')
                f = original(path, *args, **kwargs); opened.append(f); return f
            with patch.object(Path, 'open', open_file), self.assertRaises(OSError):
                RawWriters(Path(temp), ['C1', 'C2'])
            self.assertTrue(opened[0].closed)

    def test_store_constructor_and_finish_failures_close_connection(self):
        for operation in ('executescript', 'commit'):
            with self.subTest(operation=operation), tempfile.TemporaryDirectory() as temp:
                with patch('baccable_lab.storage.sqlite_store.sqlite3.connect') as connect:
                    getattr(connect.return_value, operation).side_effect = OSError('disk failed')
                    with self.assertRaises(OSError): SessionStore(Path(temp), 'test', ['C1'])
                    connect.return_value.close.assert_called_once()
        with tempfile.TemporaryDirectory() as temp:
            store = SessionStore(Path(temp), 'test', ['C1'])
            with patch.object(Path, 'write_text', side_effect=OSError('disk full')), self.assertRaises(OSError):
                store.finish('complete', 1, {})
            with self.assertRaises(sqlite3.ProgrammingError): store.database.execute('select 1')

    def test_raw_close_attempts_every_file_after_error(self):
        class Stream(io.BytesIO):
            def close(self):
                super().close()
                raise OSError('flush failed')
        streams = [io.BytesIO(), Stream()]
        with tempfile.TemporaryDirectory() as temp, patch.object(Path, 'open', side_effect=streams):
            raw = RawWriters(Path(temp), ['C1', 'C2'])
            with self.assertRaises(OSError): raw.close()
        self.assertTrue(all(stream.closed for stream in streams))


if __name__ == '__main__':
    unittest.main()
