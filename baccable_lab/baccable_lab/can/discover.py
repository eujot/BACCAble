"""Safe USB serial discovery and explicit BACCAble role mapping."""

from __future__ import annotations

from dataclasses import dataclass
from glob import glob
from pathlib import Path


@dataclass(frozen=True)
class Port:
    device: str
    description: str = ""
    vid: int | None = None
    pid: int | None = None
    serial_number: str | None = None


def list_ports() -> list[Port]:
    try:
        from serial.tools import list_ports  # type: ignore
    except ImportError:
        return [Port(path) for path in sorted(glob("/dev/cu.usbmodem*"))]
    result = []
    for item in list_ports.comports():
        result.append(Port(item.device, item.description or "", item.vid, item.pid,
                           item.serial_number))
    return sorted(result, key=lambda port: port.device)


def parse_mapping(values: list[str]) -> dict[str, str]:
    mapping: dict[str, str] = {}
    for value in values:
        try:
            role, device = value.split("=", 1)
        except ValueError as exc:
            raise ValueError(f"port mapping must be ROLE=DEVICE: {value}") from exc
        role = role.upper()
        if role not in {"C1", "C2", "BH"} or not device:
            raise ValueError(f"port mapping must use C1, C2 or BH: {value}")
        if role in mapping:
            raise ValueError(f"duplicate role mapping: {role}")
        mapping[role] = device
    return mapping


def doctor_lines() -> list[str]:
    ports = list_ports()
    if not ports:
        return ["No serial USB ports found.", "Connect BACCAble boards and run doctor again."]
    lines = ["Detected serial ports:"]
    for port in ports:
        identity = []
        if port.vid is not None and port.pid is not None:
            identity.append(f"VID:PID={port.vid:04x}:{port.pid:04x}")
        if port.serial_number:
            identity.append(f"serial={port.serial_number}")
        suffix = f" ({', '.join(identity)})" if identity else ""
        lines.append(f"  {port.device}: {port.description or 'unknown'}{suffix}")
    lines.append("Role mapping is explicit; do not assume a stable /dev path.")
    lines.append("Capture only the roles you need: --port C1=..., --port C2=... or --port BH=...; combine as needed.")
    return lines
