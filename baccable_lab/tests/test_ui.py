import queue
import threading
import unittest

from baccable_lab.can.baccable_binary import BinaryCaptureParser
from baccable_lab.capture import _DemoReader
from baccable_lab.events.catalog import EVENT_GROUPS, QUICK_MARKERS
from baccable_lab.events.search import search_markers
from baccable_lab.ui import render_dashboard


class DashboardTests(unittest.TestCase):
    def test_dashboard_shows_live_bus_data_timeline_and_grouped_markers(self):
        screen = render_dashboard(
            mode="OFFLINE PREVIEW", elapsed=12.5,
            counts={"C1": {"frames": 12, "dropped": 0}},
            latest={"C1": (11.8, 0x090, b"\x01\x02")},
            timeline=[(9.5, "ipc_menu_overwritten", "song changed")], width=110,
            selected_group="i",
        )
        for expected in ("OFFLINE PREVIEW", "DEMO", "0x090", "EVENT TIMELINE",
                         "IPC / media",
                         "Menu overwritten", "song changed", "Track changed",
                         "[g] grouped marker", "[f] find/type marker", "[1] Unlock",
                         "[7] Left indicator on"):
            self.assertIn(expected, screen)
        overview = render_dashboard(mode="LIVE CAPTURE", elapsed=0, counts={}, latest={}, timeline=[])
        for group in ("Vehicle", "Driving", "Powertrain / transmission", "Brakes / stability",
                      "Driver assistance / safety", "IPC / media", "Suspension / chassis",
                      "Weather", "wipers", "Lights", "Climate / comfort", "Electrical / diagnostics"):
            self.assertIn(group, overview)

    def test_marker_catalog_is_selectable_with_single_digit_group_indices(self):
        expected = {
            "ipc_menu_overwritten", "suspension_active", "rain_detected",
            "dipped_beam_on", "ac_on", "climate_temperature_changed",
        }
        labels = {label for _, entries in EVENT_GROUPS.values() for label, _ in entries}
        self.assertTrue(expected <= labels)
        self.assertTrue(all(len(entries) <= 10 for _, entries in EVENT_GROUPS.values()))
        self.assertGreaterEqual(len(labels), 120)
        self.assertEqual(len(QUICK_MARKERS), 10)

    def test_marker_search_handles_aliases_and_returns_stable_labels(self):
        self.assertEqual(search_markers("ABS")[0], ("abs_intervention", "ABS intervention"))
        self.assertIn(("tpms_warning", "Tyre pressure warning"), search_markers("tpms"))
        self.assertIn(("suspension_active", "Suspension active"), search_markers("ADC"))
        self.assertIn(("intercooler_temperature_high", "Intercooler temperature high"),
                      search_markers("temperatura intercoolera"))
        self.assertIn(("rain_detected", "Rain detected"), search_markers("DESZCZ"))
        self.assertEqual(search_markers("no such marker phrase"), [])
        self.assertIn(("steering_heating_changed", "Steering heating changed"),
                      search_markers("STEERING HEATING"))

    def test_expanded_group_picker_wraps_and_keeps_every_group_key_visible(self):
        screen = render_dashboard(mode="LIVE CAPTURE", elapsed=0, counts={}, latest={},
                                  timeline=[], width=60, selected_group="")
        for key in EVENT_GROUPS:
            self.assertIn(f"[{key}]", screen)

    def test_offline_reader_generates_parseable_sample_records(self):
        output = queue.Queue()
        stop, abort = threading.Event(), threading.Event()
        reader = _DemoReader("C1", "SIMULATED", output, stop, abort)
        reader.start()
        role, host_ns, payload = output.get(timeout=1)
        stop.set()
        reader.join(timeout=1)
        records = BinaryCaptureParser(role).feed(payload, host_ns)
        self.assertEqual(role, "C1")
        self.assertEqual(len(records), 1)
        self.assertIn(records[0].arbitration_id, {0x090, 0x2FA, 0x3B0, 0x5A0, 0x610})
        self.assertEqual(records[0].dlc, 8)
        self.assertEqual(len(records[0].data), 8)


if __name__ == "__main__":
    unittest.main()
