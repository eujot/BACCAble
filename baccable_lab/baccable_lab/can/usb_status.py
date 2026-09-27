"""Read-only EP0 diagnostics; never open CDC or consume binary capture records."""
from __future__ import annotations

ROLES = ("C1", "C2", "BH", "CAN")
STAGES = ("off", "detaching", "reset", "ready", "failed")
ERRORS = ("none", "init", "class", "interface", "start", "speed")


def decode_status(data: bytes) -> dict:
    if len(data) != 64 or data[0] != 1 or data[1] > 3:
        raise ValueError("unsupported USB status snapshot (expected protocol 1, 64 bytes)")
    result = {"source": ROLES[data[1]], "main_loop_ms": int.from_bytes(data[4:8], "little"),
              "pending_ack": [ROLES[i+1] for i in range(2) if data[3] & (1 << i)], "boards": {},
              "command_generation": data[14], "delivery_attempts": {"C2": data[12], "BH": data[13]}}
    for i, role in enumerate(("CAN",) if data[1] == 3 else ROLES[:3]):
        p = data[16 + i*16:32 + i*16]
        present = bool(data[2] & (1 << i))
        result["boards"][role] = {
            "fresh": present,
            "age_ms": int.from_bytes(data[8 + (i-1)*2:10 + (i-1)*2], "little") if i and data[1] == 0 else None,
            "requested_mode": p[0], "active_mode": p[1], "accepted_token": p[2],
            "usb_stage": STAGES[p[3]] if p[3] < len(STAGES) else str(p[3]),
            "last_usb_error": ERRORS[p[4]] if p[4] < len(ERRORS) else str(p[4]),
            "usb_device_state": p[5], "configured": p[5] == 3,
            "start_attempts": p[6], "reset_flags": f"0x{int.from_bytes(p[7:11], 'little'):08x}",
            "uart_tx_active": bool(p[11]), "uart_errors": int.from_bytes(p[12:14], "little"),
            "uart_recoveries": int.from_bytes(p[14:16], "little"),
        }
    return result


def read_usb_status() -> list[dict]:
    try:
        import usb.core
        import usb.util
    except ImportError as exc:
        raise ValueError("USB status needs PyUSB: install baccable_lab[usb-status] in your Python environment") from exc
    reports = []
    try:
        devices = usb.core.find(find_all=True, idVendor=0x0483)
        for device in devices:
            if device.idProduct not in (0x572a, 0x5740):
                continue
            entry = {"bus": device.bus, "address": device.address, "pid": f"{device.idProduct:04x}"}
            try:
                raw = device.ctrl_transfer(0xc0, 0x5a, 0, 0, 64, timeout=1000)
                entry.update(decode_status(bytes(raw)))
            except (usb.core.USBError, ValueError) as exc:
                entry["error"] = str(exc)
            finally:
                usb.util.dispose_resources(device)
            reports.append(entry)
    except usb.core.NoBackendError as exc:
        raise ValueError("USB status needs a libusb backend (macOS: brew install libusb)") from exc
    except usb.core.USBError as exc:
        raise ValueError(f"USB enumeration failed: {exc}") from exc
    return reports
