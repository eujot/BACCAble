from __future__ import annotations

from pathlib import Path
from contextlib import ExitStack


class RawWriters:
    def __init__(self, directory: Path, roles: list[str]):
        self._resources = ExitStack()
        try:
            self.files = {role: self._resources.enter_context((directory / f"{role}.bin").open("wb"))
                          for role in roles}
        except BaseException:
            self._resources.close()
            raise
        self.offsets = {role: 0 for role in roles}

    def write(self, role: str, data: bytes) -> int:
        offset = self.offsets[role]
        self.files[role].write(data)
        self.files[role].flush()
        self.offsets[role] += len(data)
        return offset

    def close(self) -> None:
        self._resources.close()
