"""Parser for BACCAble's fixed 16-byte binary CAN records."""

from __future__ import annotations

from dataclasses import dataclass

RECORD_SIZE = 16
TIMESTAMP_MODULUS = 1 << 24
TIMESTAMP_HALF = 1 << 23


@dataclass(frozen=True)
class CanRecord:
    role: str
    host_ns: int
    raw_timestamp: int
    device_timestamp_ms: int
    arbitration_id: int | None
    dlc: int | None
    data: bytes
    is_loss: bool = False
    dropped_count: int = 0


@dataclass
class ParserStats:
    records: int = 0
    frames: int = 0
    loss_records: int = 0
    dropped_frames: int = 0
    resyncs: int = 0
    malformed: int = 0


class BinaryCaptureParser:
    """Incrementally parse arbitrary USB read boundaries and unwrap timestamps."""

    def __init__(self, role: str):
        self.role = role
        self._buffer = bytearray()
        self._last_raw: int | None = None
        self._wraps = 0
        self.stats = ParserStats()

    @staticmethod
    def _valid_marker(value: int) -> bool:
        return 0xA0 <= value <= 0xA8 or value == 0xAF

    def _timestamp(self, raw: int) -> int:
        if self._last_raw is not None and raw < self._last_raw:
            if self._last_raw - raw > TIMESTAMP_HALF:
                self._wraps += 1
        self._last_raw = raw
        return raw + self._wraps * TIMESTAMP_MODULUS

    def feed(self, chunk: bytes, host_ns: int) -> list[CanRecord]:
        self._buffer.extend(chunk)
        output: list[CanRecord] = []
        while len(self._buffer) >= RECORD_SIZE:
            if not self._valid_marker(self._buffer[0]):
                del self._buffer[0]
                self.stats.resyncs += 1
                self.stats.malformed += 1
                continue
            record = bytes(self._buffer[:RECORD_SIZE])
            del self._buffer[:RECORD_SIZE]
            raw_timestamp = int.from_bytes(record[1:4], "little")
            timestamp = self._timestamp(raw_timestamp)
            marker = record[0]
            if marker == 0xAF:
                dropped = int.from_bytes(record[4:6], "little")
                item = CanRecord(
                    self.role, host_ns, raw_timestamp, timestamp, None, None,
                    b"", True, dropped,
                )
                self.stats.loss_records += 1
                self.stats.dropped_frames += dropped
            else:
                dlc = marker & 0x0F
                item = CanRecord(
                    self.role, host_ns, raw_timestamp, timestamp,
                    int.from_bytes(record[4:8], "little"), dlc,
                    bytes(record[8:8 + dlc]),
                )
                self.stats.frames += 1
            self.stats.records += 1
            output.append(item)
        return output

    def finish(self) -> None:
        """Mark an incomplete tail as malformed instead of silently discarding it."""
        if self._buffer:
            self.stats.malformed += len(self._buffer)
            self._buffer.clear()


def parse_bytes(role: str, payload: bytes, host_ns: int = 0) -> tuple[list[CanRecord], ParserStats]:
    parser = BinaryCaptureParser(role)
    records = parser.feed(payload, host_ns)
    parser.finish()
    return records, parser.stats
