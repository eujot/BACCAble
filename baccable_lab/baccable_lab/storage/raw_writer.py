from __future__ import annotations

from pathlib import Path


class RawWriters:
    def __init__(self, directory: Path, roles: list[str]):
        self.files = {role: (directory / f"{role}.bin").open("wb") for role in roles}
        self.offsets = {role: 0 for role in roles}

    def write(self, role: str, data: bytes) -> int:
        offset = self.offsets[role]
        self.files[role].write(data)
        self.files[role].flush()
        self.offsets[role] += len(data)
        return offset

    def close(self) -> None:
        for stream in self.files.values():
            stream.close()
