import queue
import threading
import unittest

from baccable_lab.can.baccable_binary import BinaryCaptureParser
from baccable_lab.capture import _DemoReader
from baccable_lab.events.catalog import (EVENT_GROUPS, QUICK_MARKERS, TITLE_BY_LABEL_PL,
                                         label_text_pl)
from baccable_lab.events.search import search_markers
from baccable_lab.ui import render_dashboard

SCENARIO = {
    "number": 12, "total_scenarios": 79, "completed_scenarios": 11,
    "title": "Centralny zamek: zablokowanie i odblokowanie pilotem", "location": "postój",
    "run": 2, "runs": 3, "step_index": 0, "steps_in_run": 2,
    "action": "Naciśnij UNLOCK na pilocie, poczekaj na mignięcie kierunkowskazów",
    "expected": "Odblokowanie zamków (pilot)", "paused": False,
    "handled": 3, "total": 6, "group": "Dostęp i drzwi",
    "next_action": "Naciśnij LOCK na pilocie",
    "guard": ["drzwi", "szyby"], "min_separation_s": 3, "message": "zrobione: Naciśnij LOCK (2/6)",
}


class DashboardTests(unittest.TestCase):
    def test_dashboard_shows_the_current_step_in_polish_without_a_marker_palette(self):
        screen = render_dashboard(
            mode="LIVE CAPTURE", elapsed=12.5,
            counts={"C1": {"frames": 12, "dropped": 0}},
            latest={"C1": (11.8, 0x090, b"\x01\x02")}, width=120, scenario=SCENARIO)
        for expected in ("LIVE CAPTURE", "SCENARIUSZ 12/79", "Centralny zamek", "KROK 1/2",
                         "powtórzenie 2/3", "WYKONAJ:", "Naciśnij UNLOCK", "oczekiwany stan:",
                         "Odblokowanie zamków", "postęp: 3/6", "— Dostęp i drzwi —", "dalej:  Naciśnij LOCK",
                         "[ENTER/SPACJA] zrobione", "[x] nieudane",
                         "[p] zatrzymaj", "[r] wznów", "[s] wybierz", "[k] pomiń krok",
                         "[o] pomiń scenariusz", "[e] wyklucz",
                         "nie zmieniaj: drzwi, szyby", "0x090"):
            self.assertIn(expected, screen)
        for absent in ("MARKER GROUPS", "QUICK MARKERS", "DO NOW:", "[1] Unlock"):
            self.assertNotIn(absent, screen)
        # Colour is used for readability.
        self.assertIn("\x1b[", screen)

    def test_no_color_and_caps_options(self):
        plain = render_dashboard(mode="LIVE CAPTURE", elapsed=0, counts={}, latest={},
                                 width=120, scenario=SCENARIO, color=False)
        self.assertNotIn("\x1b[", plain)
        self.assertIn("[ENTER/SPACJA] zrobione", plain)
        caps = render_dashboard(mode="LIVE CAPTURE", elapsed=0, counts={}, latest={},
                                width=120, scenario=SCENARIO, color=False, caps=True)
        self.assertIn("NACIŚNIJ UNLOCK NA PILOCIE", caps)

    def test_paused_complete_and_empty_states_are_explicit(self):
        paused = dict(SCENARIO, paused=True, action="Naciśnij LOCK na pilocie")
        screen = render_dashboard(mode="LIVE CAPTURE", elapsed=0, counts={}, latest={}, scenario=paused)
        self.assertIn("ZATRZYMANE", screen)
        complete = render_dashboard(mode="LIVE CAPTURE", elapsed=0, counts={}, latest={},
                                    scenario={"complete": True, "completed_scenarios": 79,
                                              "total_scenarios": 79, "message": ""})
        self.assertIn("WSZYSTKIE SCENARIUSZE UKOŃCZONE", complete)
        empty = render_dashboard(mode="LIVE CAPTURE", elapsed=0, counts={}, latest={}, scenario={"empty": True})
        self.assertIn("Nie wczytano biblioteki", empty)

    def test_marker_catalog_and_search_remain_available_for_scenario_labels(self):
        expected = {"ipc_menu_overwritten", "suspension_active", "rain_detected"}
        labels = {label for _, entries in EVENT_GROUPS.values() for label, _ in entries}
        self.assertTrue(expected <= labels)
        self.assertGreaterEqual(len(labels), 120)
        self.assertEqual(len(QUICK_MARKERS), 10)
        self.assertEqual(search_markers("ABS")[0], ("abs_intervention", "ABS intervention"))
        self.assertIn(("tpms_warning", "Tyre pressure warning"), search_markers("tpms"))
        self.assertEqual(search_markers("no such marker phrase"), [])
        self.assertTrue(expected <= set(TITLE_BY_LABEL_PL))
        self.assertEqual(label_text_pl("brake_on"), "Wciśnięcie pedału hamulca")

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


if __name__ == "__main__":
    unittest.main()
