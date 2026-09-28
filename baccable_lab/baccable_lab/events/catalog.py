"""Human-observed event markers for capture sessions."""

from __future__ import annotations

EVENT_GROUPS: dict[str, tuple[str, tuple[tuple[str, str], ...]]] = {
    "v": ("Vehicle", (
        ("unlock", "Unlock"), ("lock", "Lock"),
        ("driver_door_open", "Driver door open"), ("driver_door_close", "Driver door close"),
        ("passenger_door_open", "Passenger door open"), ("passenger_door_close", "Passenger door close"),
        ("engine_start", "Engine start"), ("engine_stop", "Engine stop"),
    )),
    "d": ("Driving", (
        ("brake_on", "Brake on"), ("brake_off", "Brake off"),
        ("accelerator_pressed", "Accelerator pressed"), ("accelerator_released", "Accelerator released"),
        ("parking_brake_on", "Parking brake on"), ("parking_brake_off", "Parking brake off"),
        ("reverse_selected", "Reverse selected"), ("drive_selected", "Drive selected"),
    )),
    "i": ("IPC / media", (
        ("ipc_menu_opened", "Menu opened"), ("ipc_menu_overwritten", "Menu overwritten"),
        ("ipc_menu_restored", "Menu restored"), ("ipc_screen_changed", "IPC screen changed"),
        ("track_changed", "Track changed"), ("audio_source_changed", "Audio source changed"),
        ("radio_button_pressed", "Radio button pressed"), ("phone_call_started", "Phone call started"),
        ("phone_call_ended", "Phone call ended"),
    )),
    "s": ("Suspension / chassis", (
        ("suspension_active", "Suspension active"), ("suspension_inactive", "Suspension inactive"),
        ("suspension_mode_changed", "Suspension mode changed"),
        ("ride_height_raised", "Ride height raised"), ("ride_height_lowered", "Ride height lowered"),
        ("vehicle_leveling", "Vehicle leveling"),
    )),
    "w": ("Weather / wipers", (
        ("rain_detected", "Rain detected"), ("rain_stopped", "Rain stopped"),
        ("wipers_intermittent", "Wipers intermittent"), ("wipers_slow", "Wipers slow"),
        ("wipers_fast", "Wipers fast"), ("wipers_off", "Wipers off"),
        ("washer_used", "Washer used"), ("rear_wiper_on", "Rear wiper on"),
    )),
    "l": ("Lights", (
        ("dipped_beam_on", "Dipped beam on"), ("dipped_beam_off", "Dipped beam off"),
        ("headlights_on", "Headlights on"), ("headlights_off", "Headlights off"),
        ("high_beam_on", "High beam on"), ("high_beam_off", "High beam off"),
        ("fog_lights_on", "Fog lights on"), ("fog_lights_off", "Fog lights off"),
        ("hazard_lights_on", "Hazard lights on"), ("hazard_lights_off", "Hazard lights off"),
    )),
    "a": ("Climate", (
        ("ac_on", "A/C on"), ("ac_off", "A/C off"),
        ("climate_temperature_changed", "Temperature changed"),
        ("fan_speed_changed", "Fan speed changed"),
        ("front_defrost_on", "Front defrost on"), ("rear_defrost_on", "Rear defrost on"),
        ("recirculation_on", "Recirculation on"), ("climate_mode_changed", "Climate mode changed"),
    )),
}

GROUP_BY_LABEL = {
    label: group_title
    for group_title, entries in EVENT_GROUPS.values()
    for label, _ in entries
}

# Keep established capture shortcuts stable; the complete catalog is also
# available through g → group key → item number.
QUICK_MARKERS = {
    "1": "unlock", "2": "driver_door_open", "3": "driver_door_close",
    "4": "engine_start", "5": "brake_on", "6": "brake_off",
    "7": "left_indicator_on", "8": "left_indicator_off",
    "9": "right_indicator_on", "0": "right_indicator_off",
}

MARKERS = QUICK_MARKERS | {"l": "lock", "c": "custom"}


def label_text(label: str) -> str:
    for _, entries in EVENT_GROUPS.values():
        for candidate, title in entries:
            if candidate == label:
                return title
    return label.replace("_", " ").capitalize()
