import unittest

from baccable_lab.can.baccable_binary import BinaryCaptureParser, parse_bytes


def frame(timestamp: int, can_id: int = 0x123, data: bytes = b"\x01\x02") -> bytes:
    record = bytearray(16)
    record[0] = 0xA0 | len(data)
    record[1:4] = timestamp.to_bytes(3, "little")
    record[4:8] = can_id.to_bytes(4, "little")
    record[8:8 + len(data)] = data
    return bytes(record)


class ParserTests(unittest.TestCase):
    def test_split_records_and_loss_marker(self):
        payload = frame(100) + bytes([0xAF, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
        parser = BinaryCaptureParser("C1")
        self.assertEqual(parser.feed(payload[:7], 10), [])
        records = parser.feed(payload[7:], 20)
        self.assertEqual(len(records), 2)
        self.assertEqual(records[0].arbitration_id, 0x123)
        self.assertEqual(records[1].dropped_count, 3)
        self.assertEqual(parser.stats.frames, 1)
        self.assertEqual(parser.stats.dropped_frames, 3)

    def test_timestamp_wrap_is_unwrapped(self):
        parser = BinaryCaptureParser("BH")
        records = parser.feed(frame(0xFFFFF0) + frame(0x000010), 0)
        self.assertEqual(records[1].device_timestamp_ms, (1 << 24) + 0x10)

    def test_resync_and_incomplete_tail_are_visible(self):
        records, stats = parse_bytes("C2", b"bad" + frame(4) + b"tail")
        self.assertEqual(len(records), 1)
        self.assertGreaterEqual(stats.resyncs, 3)
        self.assertEqual(stats.malformed, 7)


if __name__ == "__main__":
    unittest.main()
