import contextlib
import io
import json
import sqlite3
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from baccable_lab.capture import capture
from baccable_lab.cli import main
from baccable_lab.events.catalog import TITLE_BY_LABEL_PL
from baccable_lab.knowledge.bundle import build_bundle
from baccable_lab.knowledge.catalog import load_dictionary
from baccable_lab.knowledge.review import review_session, scenario_candidates
from baccable_lab.scenarios import (ProgressStore, ScenarioRunner, ScenarioSession,
                                    default_directory, group_label, load_scenarios, location_label,
                                    order_scenarios, parse_scenarios)
from baccable_lab.storage.schema import SCHEMA

DEMO = """
[[scenario]]
id = "demo"
title = "Demo scenario"
group = "test"
vehicle = "stationary"
runs = 2
min_separation_s = 2
confounder_guard = ["doors"]
[[scenario.steps]]
action = "Press unlock"
expect = "unlock"
[[scenario.steps]]
action = "Press lock"
expect = "lock"
"""

TWO = """
[[scenario]]
id = "alpha"
title = "Alpha"
group = "access"
vehicle = "stationary"
runs = 1
[[scenario.steps]]
action = "a"
expect = "unlock"
[[scenario]]
id = "beta"
title = "Beta"
group = "drivetrain"
vehicle = "stationary"
runs = 1
[[scenario.steps]]
action = "b"
expect = "lock"
"""


class ScenarioModelTests(unittest.TestCase):
    def test_library_loads_unique_ordered_and_fully_localized(self):
        library = load_scenarios(default_directory())
        self.assertGreaterEqual(len(library), 60)
        self.assertEqual(len(library), len({s.id for s in library.values()}))
        self.assertTrue(all(s.steps and s.runs >= 1 for s in library.values()))
        # Every scenario label has Polish on-screen wording.
        for scenario in library.values():
            for step in scenario.steps:
                if step.expect:
                    self.assertIn(step.expect, TITLE_BY_LABEL_PL)
        ordered = order_scenarios(library.values())
        groups = [s.group for s in ordered]
        self.assertLess(groups.index("access"), groups.index("drivetrain"))
        self.assertLess(groups.index("parking"), groups.index("drive"))
        self.assertLess(groups.index("drive"), groups.index("adas"))
        vehicles = [s.vehicle for s in ordered]
        first_driving = vehicles.index("driving")
        self.assertTrue(all(v != "driving" for v in vehicles[:first_driving]))
        self.assertTrue(all(v == "driving" for v in vehicles[first_driving:]))
        # A parking-lot driving set exists so the owner can record while driving.
        drive = [s for s in ordered if s.group == "drive"]
        self.assertTrue(drive and all(location_label(s) == "jazda – plac" for s in drive))

    def test_parse_rejects_unknown_label_missing_action_and_duplicates(self):
        with self.assertRaises(ValueError):
            parse_scenarios('[[scenario]]\nid="x"\n[[scenario.steps]]\naction="a"\nexpect="not_a_label"\n')
        with self.assertRaises(ValueError):
            parse_scenarios('[[scenario]]\nid="x"\n[[scenario.steps]]\nexpect="unlock"\n')
        with self.assertRaises(ValueError):
            parse_scenarios('[[scenario]]\nid="x"\n')
        with tempfile.TemporaryDirectory() as temp:
            (Path(temp) / "a.toml").write_text(DEMO)
            (Path(temp) / "b.toml").write_text(DEMO)
            with self.assertRaises(ValueError):
                load_scenarios(temp)

    def test_runner_resumes_from_a_handled_offset(self):
        scenario = parse_scenarios(DEMO)[0]      # 2 steps, 2 runs -> 4 confirmations
        runner = ScenarioRunner(scenario, handled=3)
        self.assertEqual((runner.run, runner.index), (2, 1))
        self.assertEqual(runner.progress(), (3, 4))
        self.assertFalse(runner.finished)
        pending = runner.current()
        assert pending is not None
        self.assertEqual(pending[2].expect, "lock")
        runner.done()
        self.assertTrue(runner.finished)
        self.assertIsNone(runner.current())
        self.assertIsNone(runner.done())
        self.assertEqual(ScenarioRunner(scenario, handled=4).finished, True)

    def test_runner_pause_resume_and_outcomes(self):
        runner = ScenarioRunner(parse_scenarios(DEMO)[0])
        self.assertEqual(runner.progress(), (0, 4))
        runner.pause()
        self.assertIsNone(runner.current())
        runner.resume()
        runner.done()
        runner.failed()
        self.assertEqual([r[3] for r in runner.records], ["done", "failed"])
        self.assertEqual(runner.progress(), (2, 4))


class ProgressTests(unittest.TestCase):
    def test_progress_persists_records_and_set_handled(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "scenario_state.json"
            store = ProgressStore(path)
            store.record("x", 3, 6, "done")
            store.record("x", 4, 6, "failed")
            reloaded = ProgressStore(path)
            self.assertEqual(reloaded.handled("x"), 4)
            self.assertFalse(reloaded.complete("x", 6))
            store.set_handled("x", 1, 6)
            self.assertEqual(ProgressStore(path).handled("x"), 1)

    def test_session_auto_advances_and_resumes(self):
        scenarios = parse_scenarios(TWO)
        progress = ProgressStore(None)
        session = ScenarioSession(scenarios, progress)
        runner = session.start()
        assert runner is not None
        self.assertEqual(runner.scenario.id, "alpha")
        progress.record("alpha", 1, 1, "done")
        following = session.advance()
        assert following is not None
        self.assertEqual(following.scenario.id, "beta")
        progress.record("beta", 1, 1, "done")
        self.assertIsNone(session.advance())
        self.assertEqual(session.completed(), 2)

    def test_session_resumes_first_unfinished_and_can_restart_one(self):
        scenarios = parse_scenarios(TWO)
        progress = ProgressStore(None)
        progress.record("alpha", 1, 1, "done")
        session = ScenarioSession(scenarios, progress)
        runner = session.start()
        assert runner is not None
        self.assertEqual(runner.scenario.id, "beta")
        forced = ScenarioSession(parse_scenarios(TWO), ProgressStore(None))
        self.assertEqual(forced.start("beta").scenario.id, "beta")


class ScenarioCaptureTests(unittest.TestCase):
    class Reader:
        error = None
        discarded_bytes = 0

        def __init__(self, role, device, output, stop, abort):
            self.role = role

        def start(self):
            pass

        def is_alive(self):
            return False

        def join(self, timeout):
            pass

    def run_capture(self, temp, keys, scenarios, line="", **kwargs):
        with patch("baccable_lab.capture._Reader", self.Reader), \
                patch("baccable_lab.capture._Keyboard") as keyboard, \
                contextlib.redirect_stdout(io.StringIO()):
            keyboard.return_value.read.side_effect = keys
            keyboard.return_value.read_line.return_value = line
            return capture({"C1": "/dev/test-C1"}, Path(temp), ["capture"], scenarios=scenarios, **kwargs)

    def test_auto_start_done_records_marker_and_structured_step(self):
        scenarios = {s.id: s for s in parse_scenarios(DEMO)}
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(self.run_capture(temp, ["\r", "q", "q"], scenarios), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / "session.sqlite3")) as db:
                event = db.execute("SELECT label, source FROM events").fetchone()
                step = db.execute("SELECT scenario, step_index, expect, run, outcome FROM scenario_steps").fetchone()
            self.assertEqual(event, ("unlock", "scenario"))
            self.assertEqual(step, ("demo", 0, "unlock", 1, "done"))

    def test_failed_and_pause_resume_are_recorded(self):
        scenarios = {s.id: s for s in parse_scenarios(DEMO)}
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(self.run_capture(temp, ["p", "r", "x", "q", "q"], scenarios), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / "session.sqlite3")) as db:
                rows = db.execute("SELECT expect, outcome FROM scenario_steps ORDER BY id").fetchall()
            self.assertEqual(rows, [("unlock", "failed")])

    def test_progress_resumes_a_new_capture_run(self):
        scenarios = {s.id: s for s in parse_scenarios(DEMO)}
        with tempfile.TemporaryDirectory() as temp:
            state = Path(temp) / "state.json"
            # First run handles one step, then quits.
            with tempfile.TemporaryDirectory() as root:
                self.assertEqual(self.run_capture(root, ["\r", "q", "q"], scenarios,
                                                  progress=ProgressStore(state)), 0)
            self.assertEqual(ProgressStore(state).handled("demo"), 1)
            # A new run resumes at the second step (lock) instead of the first.
            with tempfile.TemporaryDirectory() as root:
                self.assertEqual(self.run_capture(root, ["\r", "q", "q"], scenarios,
                                                  progress=ProgressStore(state)), 0)
                directory, = Path(root).iterdir()
                with contextlib.closing(sqlite3.connect(directory / "session.sqlite3")) as db:
                    expect, run, index = db.execute(
                        "SELECT expect, run, step_index FROM scenario_steps").fetchone()
            self.assertEqual((expect, run, index), ("lock", 1, 1))

    def test_operator_note_is_stored_on_the_step(self):
        scenarios = {s.id: s for s in parse_scenarios(DEMO)}
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(self.run_capture(temp, ["\r", "n", "q", "q"], scenarios, line="silnik zgasł"), 0)
            directory, = Path(temp).iterdir()
            with contextlib.closing(sqlite3.connect(directory / "session.sqlite3")) as db:
                step_note = db.execute("SELECT note FROM scenario_steps").fetchone()[0]
                event_note = db.execute("SELECT note FROM events").fetchone()[0]
            self.assertEqual(step_note, "silnik zgasł")
            self.assertEqual(event_note, "silnik zgasł")

    def test_unknown_scenario_is_rejected_without_creating_a_session(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "sessions"
            with self.assertRaises(ValueError):
                capture({"C1": "/dev/test-C1"}, root, [], scenarios={}, scenario="missing")
            self.assertFalse(root.exists())

    def test_context_is_written_into_the_manifest_and_summary(self):
        scenarios = {s.id: s for s in parse_scenarios(DEMO)}
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(self.run_capture(temp, ["\r", "q", "q"], scenarios,
                                              context={"vehicle": "Giulia", "firmware": "beta20"}), 0)
            directory, = Path(temp).iterdir()
            manifest = json.loads((directory / "manifest.json").read_text())
            summary = json.loads((directory / "summary.json").read_text())
            self.assertEqual(manifest["context"], {"vehicle": "Giulia", "firmware": "beta20"})
            self.assertEqual(summary["context"], {"vehicle": "Giulia", "firmware": "beta20"})

    def test_bundle_is_a_self_contained_analysis_artifact(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp) / "sess"
            directory.mkdir()
            database = sqlite3.connect(directory / "session.sqlite3")
            database.executescript(SCHEMA)
            database.execute("INSERT INTO sessions VALUES ('sess','now','later',10,'complete','monotonic')")
            database.execute("INSERT INTO events VALUES (1,'sess',1000000,'unlock','scenario','note')")
            database.execute("INSERT INTO scenario_steps(session_id,host_ns,event_id,scenario,step_index,"
                             "step_key,expect,run,outcome) VALUES ('sess',1000000,1,'lock_unlock',0,'','unlock',1,'done')")
            for index in range(4):
                database.execute("INSERT INTO can_frames(session_id,role,sequence,host_ns,device_timestamp_raw,"
                                 "device_timestamp_ms,arbitration_id,dlc,data,raw_offset) "
                                 "VALUES ('sess','C1',?,?,0,?,?,8,?,0)",
                                 (index, 1_000_000 + index, index, 0x123, bytes(8)))
            database.commit()
            database.close()
            (directory / "manifest.json").write_text(json.dumps({"context": {"vehicle": "Giulia"}}))
            library = load_scenarios(default_directory())
            bundle = build_bundle(directory, load_dictionary(), scenarios=library)
            self.assertEqual(bundle["session"]["status"], "complete")
            self.assertEqual(bundle["context"], {"vehicle": "Giulia"})
            self.assertEqual(bundle["scenario"]["steps"][0]["expect"], "unlock")
            self.assertEqual(bundle["scenario"]["steps"][0]["guard"], ["drzwi", "szyby", "zapłon", "światła"])
            self.assertEqual(bundle["scenario"]["steps"][0]["runs"], 3)
            self.assertIn("lock_unlock", bundle["scenario"]["library"])
            self.assertEqual(bundle["coverage"]["unmapped_frames"], 4)
            self.assertTrue(bundle["shape_hints"][0]["constant"])
            self.assertIn("Bundle sess", bundle["brief"])
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(main(["bundle", "sess", "--sessions", temp, "--output",
                                       str(Path(temp) / "b.json")]), 0)
            self.assertTrue((Path(temp) / "b.json").exists())

    def test_review_reports_scenario_steps(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp) / "test"
            directory.mkdir()
            database = sqlite3.connect(directory / "session.sqlite3")
            database.executescript(SCHEMA)
            database.execute("INSERT INTO sessions VALUES ('test','now','later',3,'complete','monotonic')")
            database.execute("INSERT INTO events VALUES (1,'test',1000000,'unlock','scenario','note')")
            database.execute("INSERT INTO scenario_steps(session_id,host_ns,event_id,scenario,step_index,"
                             "step_key,expect,run,outcome) VALUES ('test',1000000,1,'lock_unlock',0,'','unlock',1,'done')")
            database.commit()
            database.close()
            result = review_session(directory, load_dictionary())
            self.assertEqual(result["scenario_steps"][0]["scenario"], "lock_unlock")
            self.assertEqual(result["scenario_overview"], [{"scenario": "lock_unlock", "run": 1,
                                                            "done": 1, "failed": 0}])

    def test_group_labels_and_scenario_candidate_ranking(self):
        self.assertEqual(group_label("access"), "Dostęp i drzwi")
        self.assertEqual(group_label("drive"), "Jazda na placu")
        steps = [{"role": "C1", "can_id": "0x101", "byte": 0, "bit": 2, "label": "brake_on",
                  "event_id": n, "known_fields": [], "before": 0, "after": 1,
                  "one_fraction_before_after": [0.0, 1.0], "samples_before_after": [200, 200]}
                 for n in (1, 2, 3)]
        steps.append({"role": "C1", "can_id": "0x101", "byte": 1, "bit": 0, "label": "brake_on",
                      "event_id": 4, "known_fields": [], "before": 0, "after": 1,
                      "one_fraction_before_after": [0.0, 1.0], "samples_before_after": [200, 200]})
        ranked = scenario_candidates(steps)
        self.assertEqual(len(ranked), 1)
        self.assertEqual(ranked[0]["bit"], 2)
        self.assertEqual(len(ranked[0]["event_ids"]), 3)
        self.assertEqual(scenario_candidates([]), [])


class DictionaryProposalTests(unittest.TestCase):
    def test_propose_outputs_validated_candidate(self):
        with contextlib.redirect_stdout(io.StringIO()) as output, contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(main(["dictionary", "--propose", "--role", "C1", "--id", "0x123",
                                   "--name", "demo_signal", "--parts", "1,0,8", "--unit", "raw"]), 0)
        message = json.loads(output.getvalue())
        self.assertEqual(message["signals"][0]["name"], "demo_signal")
        self.assertEqual(message["signals"][0]["parts"], [[1, 0, 8]])
        with contextlib.redirect_stderr(io.StringIO()) as error:
            self.assertEqual(main(["dictionary", "--propose", "--role", "C1", "--id", "0x123",
                                   "--name", "bad", "--parts", "9,0,8"]), 2)
        self.assertIn("error", error.getvalue())

    def test_accept_merges_into_a_draft_file(self):
        with tempfile.TemporaryDirectory() as temp:
            proposal = Path(temp) / "proposal.json"
            proposal.write_text(json.dumps({"role": "C1", "can_id": "0x7FF", "title": "Demo",
                                            "signals": [{"name": "s", "parts": [[0, 0, 8]], "unit": "raw",
                                                          "scale": 1, "offset": 0, "origin": "capture"}]}))
            draft = Path(temp) / "draft.json"
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(main(["dictionary", "--accept", str(proposal), "--output", str(draft)]), 0)
            merged = json.loads(draft.read_text())
            self.assertTrue(any(m["can_id"] == "0x7FF" for m in merged["messages"]))
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(main(["dictionary", "--accept", str(proposal), "--output", str(draft)]), 2)


class ScenarioCliTests(unittest.TestCase):
    def test_scenarios_command_lists_order_and_progress(self):
        with tempfile.TemporaryDirectory() as temp:
            state = str(Path(temp) / "state.json")
            with contextlib.redirect_stdout(io.StringIO()) as output:
                self.assertEqual(main(["scenarios", "--state", state]), 0)
            text = output.getvalue()
            self.assertIn("lock_unlock", text)
            self.assertIn("scenariuszy ukończonych", text)
            with contextlib.redirect_stdout(io.StringIO()) as output:
                self.assertEqual(main(["scenarios", "--json", "--state", state]), 0)
        entries = json.loads(output.getvalue())
        self.assertTrue(any(s["id"] == "lock_unlock" for s in entries))
        self.assertTrue(all("steps" in s and s["steps"] and "complete" in s and "location" in s
                            for s in entries))
        self.assertEqual([s["group"] for s in entries if s["id"] == "lock_unlock"], ["access"])

    def test_capture_accepts_scenario_flag_and_writes_progress(self):
        with tempfile.TemporaryDirectory() as temp:
            with patch("baccable_lab.capture._Reader", ScenarioCaptureTests.Reader), \
                    patch("baccable_lab.capture._Keyboard") as keyboard, \
                    contextlib.redirect_stdout(io.StringIO()):
                keyboard.return_value.read.side_effect = ["\r", "q", "q"]
                state = Path(temp) / "state.json"
                self.assertEqual(main(["capture", "--port", "C1=/dev/test-C1", "--sessions", temp,
                                       "--scenario", "lock_unlock", "--state", str(state)]), 0)
            directory, = (p for p in Path(temp).iterdir() if p.is_dir())
            with contextlib.closing(sqlite3.connect(directory / "session.sqlite3")) as db:
                self.assertEqual(db.execute("SELECT scenario, outcome FROM scenario_steps").fetchone(),
                                 ("lock_unlock", "done"))
            self.assertEqual(ProgressStore(state).handled("lock_unlock"), 1)


if __name__ == "__main__":
    unittest.main()
