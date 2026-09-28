"""Versioned, passive CAN knowledge. Code evidence is not vehicle validation."""
from __future__ import annotations

import json
from importlib.resources import files


def load_dictionary() -> dict:
    catalog = json.loads(files(__package__).joinpath("giulia.json").read_text(encoding="utf-8"))
    validate_dictionary(catalog)
    return catalog


def validate_dictionary(catalog: dict) -> None:
    if catalog["schema_version"] != 1:
        raise ValueError("unsupported dictionary schema")
    seen = set()
    for message in catalog["messages"]:
        key = (message["role"], int(message["can_id"], 16))
        if key[0] not in ("C1", "C2", "BH") or not 0 <= key[1] <= 0x1FFFFFFF or key in seen:
            raise ValueError(f"invalid or duplicate message: {key}")
        seen.add(key)
        names = set()
        for signal in message["signals"]:
            if signal["name"] in names or signal["origin"] not in ("implementation", "comment", "capture"):
                raise ValueError("invalid signal identity or origin")
            names.add(signal["name"])
            if not signal["parts"]:
                raise ValueError("empty bit layout")
            for byte, lsb, width in signal["parts"]:
                if not (0 <= byte < 8 and 0 <= lsb < 8 and 1 <= width <= 8 - lsb):
                    raise ValueError("invalid bit layout")
    parameter_ids = [p["parameter_id"] for p in catalog["diagnostic_parameters"]]
    if len(set(parameter_ids)) != len(parameter_ids):
        raise ValueError("duplicate diagnostic parameter")
    templates = set()
    for t in catalog.get('frame_templates', []):
        key = (t['role'],int(t['can_id'],16),t['dlc'])
        if key in templates or key[0] not in ('C1','C2','BH') or not 0 <= key[2] <= 8:
            raise ValueError('invalid template identity')
        templates.add(key)
        if len(bytes.fromhex(t['stable_bit_masks'])) != t['dlc'] or len(bytes.fromhex(t['stable_values'])) != t['dlc']:
            raise ValueError('invalid template width')
        if int(t['stable_values'] or '0',16) & ~int(t['stable_bit_masks'] or '0',16):
            raise ValueError('template values outside stable mask')


def extract_signal(signal: dict, data: bytes) -> dict | None:
    if any(byte >= len(data) for byte, _, _ in signal["parts"]):
        return None
    raw = 0
    for byte, lsb, width in signal["parts"]:
        raw = (raw << width) | ((data[byte] >> lsb) & ((1 << width) - 1))
    valid = raw not in signal.get("invalid_raw", [])
    return {"name": signal["name"], "raw": raw,
            "value": raw * signal["scale"] + signal["offset"] if valid else None,
            "unit": signal["unit"], "enum": signal.get("enum", {}).get(str(raw)),
            "origin": signal["origin"], "valid": valid}


def decode_frame(catalog: dict, role: str, can_id: int, data: bytes) -> dict:
    if role not in ("C1", "C2", "BH") or not 0 <= can_id <= 0x1FFFFFFF or len(data) > 8:
        raise ValueError("invalid classical CAN frame")
    message = next((m for m in catalog["messages"]
                    if m["role"] == role and int(m["can_id"], 16) == can_id), None)
    signals = []
    if message is not None and len(data) == message.get("exact_dlc", len(data)):
        signals = [result for s in message["signals"] if (result := extract_signal(s, data)) is not None]
    diagnostics = []
    for parameter in catalog["diagnostic_parameters"]:
        if parameter["role"] != role or int(parameter["response_id"], 16) != can_id:
            continue
        # Match the firmware single-frame decoder; never interpret ISO-TP FF/CF or NRC as a value.
        if not 4 <= len(data) <= 8 or not 3 <= data[0] <= 7 or data[0] + 1 > len(data):
            continue
        if data[1] != 0x62 or int.from_bytes(data[2:4], "big") != int(parameter["did"], 16):
            continue
        start = 4 + parameter["value_offset"]
        end = start + parameter["width"]
        if not 1 <= parameter["width"] <= 4 or end > data[0] + 1:
            continue
        raw = int.from_bytes(data[start:end], "big")
        diagnostics.append({"parameter_id": parameter["parameter_id"], "did": parameter["did"],
                            "raw": raw, "value": (raw + parameter["raw_offset"]) * parameter["scale"]
                            + parameter["scaled_offset"], "menu_pages": parameter["menu_pages"]})
    baseline = None
    for template in catalog.get('frame_templates', []):
        if (template['role'],int(template['can_id'],16),template['dlc']) == (role,can_id,len(data)):
            mask = int(template['stable_bit_masks'] or '0',16)
            expected = int(template['stable_values'] or '0',16)
            baseline = {'status':'no_observed_stable_bits' if not mask else
                        'matches_observed_stable_bits' if int.from_bytes(data,'big') & mask == expected
                        else 'differs_from_observed_stable_bits', 'sessions':template['sessions'],
                        'limits':'An empirical template match does not identify an ECU or validate a function.'}
            break
    return {"role": role, "can_id": f"0x{can_id:03X}", "title": message["title"] if message else None,
            "data": data.hex().upper(), "signals": signals, "diagnostics": diagnostics, 'baseline':baseline}


def crc8_j1850(data: bytes) -> int:
    crc = 0xFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ (0x1D if crc & 0x80 else 0)) & 0xFF
    return crc ^ 0xFF
