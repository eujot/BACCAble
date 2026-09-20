import sqlite3
import tempfile
import unittest
from pathlib import Path

from baccable_lab.can.baccable_binary import parse_bytes
from baccable_lab.storage.sqlite_store import SessionStore


class StorageTests(unittest.TestCase):
    def test_frames_loss_and_events_survive_reopen(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp) / "session"
            directory.mkdir()
            store = SessionStore(directory, "session", ["C1", "C2", "BH"])
            records, _ = parse_bytes("C1", bytes([0xA1, 1, 0, 0, 0x34, 0x12, 0, 0, 0xAA] + [0] * 7))
            store.add_record(records[0], 0)
            store.add_event(42, "engine_start")
            store.finish("complete", 1.5, {"ok": True})
            database = sqlite3.connect(directory / "session.sqlite3")
            self.assertEqual(database.execute("SELECT count(*) FROM can_frames").fetchone()[0], 1)
            self.assertEqual(database.execute("SELECT label FROM events").fetchone()[0], "engine_start")
            database.close()


if __name__ == "__main__":
    unittest.main()
