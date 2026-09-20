from __future__ import annotations

import json
from pathlib import Path


def write_manifest(directory: Path, session_id: str, ports: dict[str, str], args: list[str]) -> None:
    (directory / "manifest.json").write_text(json.dumps({
        "session_id": session_id,
        "roles": ports,
        "command": args,
        "raw_record_size": 16,
        "capture_format": "BACCAble binary CAN",
    }, indent=2) + "\n")
