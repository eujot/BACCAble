"""Human-observed event markers for capture sessions.

The catalog describes useful moments to annotate, not signals decoded from CAN.
Availability and names of vehicle functions vary by model, year, market and trim.
"""

from __future__ import annotations

EVENT_GROUPS: dict[str, tuple[str, tuple[tuple[str, str], ...]]] = {
    "v": ("Vehicle", (
        ("unlock", "Unlock"), ("lock", "Lock"),
        ("driver_door_open", "Driver door open"), ("driver_door_close", "Driver door close"),
        ("passenger_door_open", "Passenger door open"), ("passenger_door_close", "Passenger door close"),
        ("engine_start", "Engine start"), ("engine_stop", "Engine stop"),
        ("ignition_on", "Ignition on"), ("ignition_off", "Ignition off"),
    )),
    "d": ("Driving", (
        ("accelerator_pressed", "Accelerator pressed"), ("accelerator_released", "Accelerator released"),
        ("cruise_control_set", "Cruise control set"), ("cruise_control_resumed", "Cruise resumed"),
        ("cruise_control_cancelled", "Cruise cancelled"), ("speed_changed", "Speed changed"),
        ("hill_start", "Hill start"), ("wheel_slip", "Wheel slip"),
        ("rough_road", "Rough road"), ("road_event", "Road event"),
    )),
    "p": ("Powertrain / transmission", (
        ("reverse_selected", "Reverse selected"), ("neutral_selected", "Neutral selected"),
        ("park_selected", "Park selected"), ("drive_selected", "Drive selected"),
        ("gear_up", "Gear up"), ("gear_down", "Gear down"),
        ("manual_mode_on", "Manual mode on"), ("dna_mode_changed", "DNA mode changed"),
        ("start_stop_active", "Start/Stop active"), ("start_stop_restarted", "Start/Stop restarted"),
    )),
    "t": ("Engine / live readings", (
        ("engine_rpm_changed", "Engine RPM changed"), ("engine_load_changed", "Engine load changed"),
        ("boost_changed", "Boost changed"), ("coolant_temperature_high", "Coolant temperature high"),
        ("oil_temperature_high", "Oil temperature high"),
        ("intercooler_temperature_high", "Intercooler temperature high"),
        ("oil_pressure_warning", "Oil pressure warning"), ("battery_soc_changed", "Battery SOC changed"),
        ("fuel_level_changed", "Fuel level changed"), ("cooling_fan_active", "Cooling fan active"),
    )),
    "b": ("Brakes / stability", (
        ("brake_on", "Brake on"), ("brake_off", "Brake off"),
        ("parking_brake_on", "Parking brake on"), ("parking_brake_off", "Parking brake off"),
        ("abs_intervention", "ABS intervention"), ("esc_intervention", "ESC intervention"),
        ("traction_control_intervention", "Traction control intervention"),
        ("hill_descent_active", "Hill descent active"), ("brake_warning", "Brake warning"),
        ("brake_hold_active", "Brake hold active"),
    )),
    "x": ("Driver assistance / safety", (
        ("acc_set", "ACC set"), ("acc_following", "ACC following"),
        ("acc_resume", "ACC resumed"), ("acc_cancelled", "ACC cancelled"),
        ("forward_collision_warning", "Forward collision warning"),
        ("automatic_emergency_braking", "Automatic emergency braking"),
        ("blind_spot_warning", "Blind spot warning"), ("lane_warning", "Lane warning"),
        ("lane_steering_intervention", "Lane steering intervention"),
        ("driver_attention_warning", "Driver attention warning"),
    )),
    "o": ("Body / access", (
        ("trunk_open", "Trunk open"), ("trunk_close", "Trunk close"),
        ("hood_open", "Hood open"), ("fuel_door_open", "Fuel door open"),
        ("window_open", "Window opened"), ("window_close", "Window closed"),
        ("mirror_folded", "Mirror folded"), ("mirror_unfolded", "Mirror unfolded"),
        ("seat_belt_fastened", "Seat belt fastened"), ("seat_belt_unfastened", "Seat belt unfastened"),
    )),
    "i": ("IPC / media", (
        ("ipc_menu_opened", "Menu opened"), ("ipc_menu_overwritten", "Menu overwritten"),
        ("ipc_menu_restored", "Menu restored"), ("ipc_screen_changed", "IPC screen changed"),
        ("track_changed", "Track changed"), ("audio_source_changed", "Audio source changed"),
        ("radio_button_pressed", "Radio button pressed"), ("phone_call_started", "Phone call started"),
        ("phone_call_ended", "Phone call ended"), ("navigation_prompt", "Navigation prompt"),
    )),
    "s": ("Suspension / chassis", (
        ("suspension_active", "Suspension active"), ("suspension_inactive", "Suspension inactive"),
        ("suspension_mode_changed", "Suspension mode changed"),
        ("ride_height_raised", "Ride height raised"), ("ride_height_lowered", "Ride height lowered"),
        ("vehicle_leveling", "Vehicle leveling"), ("awd_active", "AWD active"),
        ("steering_mode_changed", "Steering mode changed"), ("steering_input", "Steering input"),
    )),
    "w": ("Weather / wipers", (
        ("rain_detected", "Rain detected"), ("rain_stopped", "Rain stopped"),
        ("wipers_intermittent", "Wipers intermittent"), ("wipers_slow", "Wipers slow"),
        ("wipers_fast", "Wipers fast"), ("wipers_off", "Wipers off"),
        ("washer_used", "Washer used"), ("rear_wiper_on", "Rear wiper on"),
        ("ambient_light_changed", "Ambient light changed"), ("outside_temperature_changed", "Outside temperature changed"),
    )),
    "l": ("Lights", (
        ("left_indicator_on", "Left indicator on"), ("left_indicator_off", "Left indicator off"),
        ("right_indicator_on", "Right indicator on"), ("right_indicator_off", "Right indicator off"),
        ("dipped_beam_on", "Dipped beam on"), ("dipped_beam_off", "Dipped beam off"),
        ("headlights_on", "Headlights on"), ("high_beam_on", "High beam on"),
        ("fog_lights_on", "Fog lights on"), ("hazard_lights_on", "Hazard lights on"),
    )),
    "h": ("Lighting / visibility", (
        ("side_lights_on", "Side lights on"), ("side_lights_off", "Side lights off"),
        ("automatic_headlights_on", "Automatic headlights on"),
        ("automatic_headlights_off", "Automatic headlights off"),
        ("high_beam_off", "High beam off"), ("fog_lights_off", "Fog lights off"),
        ("hazard_lights_off", "Hazard lights off"),
        ("interior_lights_on", "Interior lights on"), ("interior_lights_off", "Interior lights off"),
        ("brake_lights_on", "Brake lights on"),
    )),
    "a": ("Climate / comfort", (
        ("ac_on", "A/C on"), ("ac_off", "A/C off"),
        ("climate_temperature_changed", "Temperature changed"), ("fan_speed_changed", "Fan speed changed"),
        ("front_defrost_on", "Front defrost on"), ("rear_defrost_on", "Rear defrost on"),
        ("recirculation_on", "Recirculation on"), ("climate_mode_changed", "Climate mode changed"),
        ("seat_heating_changed", "Seat heating changed"), ("steering_heating_changed", "Steering heating changed"),
    )),
    "k": ("Parking / manoeuvring", (
        ("park_sensors_active", "Parking sensors active"), ("park_sensor_warning", "Parking sensor warning"),
        ("reverse_camera_active", "Reverse camera active"), ("park_assist_started", "Park assist started"),
        ("park_assist_finished", "Park assist finished"), ("auto_park_brake", "Auto parking brake"),
        ("door_open_warning", "Door open warning"), ("vehicle_approach", "Vehicle approach"),
    )),
    "e": ("Electrical / diagnostics", (
        ("module_wake", "Module wake"), ("module_sleep", "Module sleep"),
        ("warning_message", "Warning message"), ("fault_message", "Fault message"),
        ("warning_cleared", "Warning cleared"), ("tpms_warning", "Tyre pressure warning"),
        ("battery_warning", "Battery warning"), ("charging_changed", "Charging changed"),
        ("obd_connected", "OBD connected"), ("diagnostic_session", "Diagnostic session"),
    )),
}

GROUP_BY_LABEL = {
    label: group_title
    for group_title, entries in EVENT_GROUPS.values()
    for label, _ in entries
}
TITLE_BY_LABEL = {
    label: title
    for _, entries in EVENT_GROUPS.values()
    for label, title in entries
}

# Search aliases include common names and abbreviations that may not appear in
# the visible marker wording. These are lookup hints, not decoded CAN values.
SEARCH_ALIASES: dict[str, tuple[str, ...]] = {
    "unlock": ("odblokuj", "odblokowanie"), "lock": ("zablokuj", "zamknij auto"),
    "driver_door_open": ("otwórz drzwi kierowcy", "drzwi kierowcy otwarte"),
    "driver_door_close": ("zamknij drzwi kierowcy", "drzwi kierowcy zamknięte"),
    "engine_start": ("uruchom silnik", "odpal auto"), "engine_stop": ("zgaś silnik", "wyłącz silnik"),
    "brake_on": ("hamulec", "hamowanie", "wciśnij hamulec"),
    "left_indicator_on": ("lewy kierunkowskaz", "lewy kierunek"),
    "left_indicator_off": ("lewy kierunkowskaz", "lewy kierunek"),
    "right_indicator_on": ("prawy kierunkowskaz", "prawy kierunek"),
    "right_indicator_off": ("prawy kierunkowskaz", "prawy kierunek"),
    "rain_detected": ("deszcz", "czujnik deszczu", "rain sensor", "wipers", "windshield", "windscreen"),
    "wipers_intermittent": ("wycieraczki", "wycieraczki przerywane"),
    "ac_on": ("klimatyzacja", "klima", "hvac", "air conditioning", "aircon"),
    "intercooler_temperature_high": ("temperatura intercoolera", "temperatura interkulera",
                                     "intercooler temp", "charge air temperature", "iat"),
    "coolant_temperature_high": ("temperatura płynu chłodzącego", "temperatura silnika",
                                  "coolant temp", "engine temperature", "water temperature"),
    "battery_soc_changed": ("stan naładowania", "soc", "state of charge", "state-of-charge", "battery charge"),
    "suspension_active": ("aktywne zawieszenie", "adc", "active damping control"),
    "dna_mode_changed": ("dna", "dynamic normal advanced efficiency race"),
    "start_stop_active": ("s&s", "start stop", "stop start"),
    "abs_intervention": ("abs",), "esc_intervention": ("esc", "esp"),
    "traction_control_intervention": ("asr", "tcs", "tc"),
    "acc_set": ("acc", "adaptive cruise control"),
    "acc_following": ("acc", "adaptive cruise control", "follow"),
    "forward_collision_warning": ("fcw",),
    "automatic_emergency_braking": ("aeb", "autonomous emergency braking"),
    "blind_spot_warning": ("bsw", "bsm", "blind spot monitoring"),
    "lane_warning": ("ldw", "lane departure warning", "lka", "lane keeping assist"),
    "lane_steering_intervention": ("lka", "lane keeping assist"),
    "driver_attention_warning": ("daa", "driver attention assist"),
    "suspension_mode_changed": ("adc", "active damping control"),
    "awd_active": ("q4", "awd", "4wd", "four wheel drive"),
    "tpms_warning": ("tpms", "tire pressure", "tyre pressure"),
    "park_sensors_active": ("pdc", "parking distance control", "parking sensor"),
    "climate_temperature_changed": ("hvac", "temperature", "air conditioning"),
    "ipc_menu_overwritten": ("instrument panel cluster", "radio", "screen"),
    "module_wake": ("wakeup", "network start", "ignition"),
    "module_sleep": ("sleep", "network stop", "ignition off"),
    "engine_rpm_changed": ("rpm", "revolutions"), "engine_load_changed": ("engine load",),
    "boost_changed": ("boost pressure", "turbo", "turbo pressure"),
    "oil_temperature_high": ("oil temp",),
    "oil_pressure_warning": ("oil pressure",),
    "cooling_fan_active": ("radiator fan", "fan active",),
}

# Preserve the established one-key capture actions.
QUICK_MARKERS = {
    "1": "unlock", "2": "driver_door_open", "3": "driver_door_close",
    "4": "engine_start", "5": "brake_on", "6": "brake_off",
    "7": "left_indicator_on", "8": "left_indicator_off",
    "9": "right_indicator_on", "0": "right_indicator_off",
}

MARKERS = QUICK_MARKERS | {"l": "lock", "c": "custom"}


def label_text(label: str) -> str:
    return TITLE_BY_LABEL.get(label, label.replace("_", " ").capitalize())
