#!/usr/bin/env python3
"""Scoped vehicle-UI test runner.

Runs only the exact cases declared in a manifest; it never defaults to the broad
host suite. Each selected case is passed to the host runner through the
BACCABLE_TEST_CASES environment allowlist.
"""
import argparse
import json
import os
import re
import subprocess
import sys

SCHEMA = 1
TESTS_DIR = os.path.dirname(os.path.abspath(__file__))

# Build target -> allowed variants. Rejects a legacy18 target labeled my23 and
# rejects arbitrary build targets that are not part of this scoped workflow.
VARIANT_EXPECT = {
    "build/test_vehicle_profile": {"legacy18"},
    "build/test_menu": {"legacy18"},
    "build/test_runner_selection": {"selection"},
    "build/test_reading_catalog": {"legacy18"},
    "build/test_reading_layout": {"legacy18"},
    "build/test_favorite_layout": {"legacy18"},
    "build/test_ui_layout_store": {"legacy18"},
}

BUILD_RE = re.compile(r"^build/[A-Za-z0-9_]+$")
SOURCE_RE = re.compile(r"^[A-Za-z0-9_]+\.c$")


def fail(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def load_manifest(path):
    if not path:
        fail("a --manifest path is required")
    if not os.path.isfile(path):
        fail(f"manifest not found: {path}")
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    if data.get("schema") != SCHEMA:
        fail(f"unsupported manifest schema: {data.get('schema')!r}")
    runs = data.get("runs")
    if not isinstance(runs, list) or not runs:
        fail("manifest must declare a non-empty runs list")
    return data, runs


def validate_runs(runs):
    seen_cases = {}
    seen_builds = set()
    for run in runs:
        target = run.get("build_target", "")
        variant = run.get("variant", "")
        source = run.get("source", "")
        cases = run.get("cases", [])
        if target not in VARIANT_EXPECT:
            fail(f"unknown build_target {target!r}")
        if variant not in VARIANT_EXPECT[target]:
            fail(f"variant {variant!r} not allowed for {target}")
        if not BUILD_RE.match(target):
            fail(f"invalid build_target {target!r}")
        if not SOURCE_RE.match(source):
            fail(f"invalid source {source!r}")
        if not isinstance(cases, list) or not cases:
            fail(f"run {target}/{variant} has no cases")
        key = (target, variant)
        if key in seen_builds:
            fail(f"duplicate run {target}/{variant}")
        seen_builds.add(key)
        for case in cases:
            name = case.get("name", "")
            if not name:
                fail(f"run {target}/{variant} has a case without a name")
            if name in seen_cases:
                fail(f"duplicate case name {name!r}")
            seen_cases[name] = key
    return seen_cases


def list_runs(runs):
    for run in runs:
        names = ", ".join(c["name"] for c in run["cases"])
        print(f"{run['build_target']}\t{run['variant']}\t{run['source']}\t{names}")


def build_target(target):
    cmd = ["/usr/bin/make", "-C", TESTS_DIR, "CC=/usr/bin/clang", target]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if proc.returncode != 0:
        print(proc.stdout, file=sys.stderr)
        fail(f"build failed for {target} (exit {proc.returncode})")
    return os.path.join(TESTS_DIR, target)


def run_case_list(binary, names):
    env = os.environ.copy()
    env["BACCABLE_TEST_CASES"] = ",".join(names)
    proc = subprocess.run([binary], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return proc.returncode, proc.stdout


def run_manifest(runs):
    targets = sorted({r["build_target"] for r in runs})
    for target in targets:
        build_target(target)

    total = 0
    for run in runs:
        binary = os.path.join(TESTS_DIR, run["build_target"])
        names = [c["name"] for c in run["cases"]]
        code, out = run_case_list(binary, names)
        run_events = [line for line in out.splitlines() if line.startswith("CASE RUN ")]
        pass_events = [line for line in out.splitlines() if line.startswith("CASE PASS ")]
        missing = [line for line in out.splitlines() if line.startswith("CASE MISSING ")]

        ran = {line.rsplit(" ", 1)[-1] for line in run_events}
        passed = {line.rsplit(" ", 1)[-1] for line in pass_events}
        expected = set(names)

        if code != 0:
            print(out, file=sys.stderr)
            fail(f"{run['build_target']} exited {code}")
        if missing:
            print(out, file=sys.stderr)
            fail(f"declared case missing in {run['build_target']}")
        if ran != expected or passed != expected:
            print(out, file=sys.stderr)
            fail(
                f"case accounting mismatch in {run['build_target']}/{run['variant']}: "
                f"expected {sorted(expected)}, ran {sorted(ran)}, passed {sorted(passed)}"
            )

        total += len(names)
        print(f"PASS {run['build_target']} {run['variant']} {len(names)} cases")

    print(f"vehicle-ui scoped run: {total} selected cases passed")
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--list", action="store_true", help="dry-run: print runs without executing test code")
    parser.add_argument("--run", action="store_true", help="build and run only the selected cases")
    args = parser.parse_args()

    data, runs = load_manifest(args.manifest)
    validate_runs(runs)

    if args.list:
        print(f"manifest stage={data.get('stage')!r} schema={SCHEMA}")
        list_runs(runs)
        return 0
    if args.run:
        return run_manifest(runs)

    parser.error("choose --list or --run")


if __name__ == "__main__":
    sys.exit(main())
