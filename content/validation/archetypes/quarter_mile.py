#!/usr/bin/env python3
"""Writes a drag-run scenario for each archetype whose gearbox is shifted by the driver (manual on the paddles,
sequential, dual clutch): content/validation/scenarios/qm/<slug>.json, and points its validation's quarter-mile
targets at it.

    python3 content/validation/archetypes/quarter_mile.py [--bin build-dev/bin] [--sim DIR] [slug ...]

The run is the car's own: its gearbox, its clutch (the paddles' robotised one or a dual clutch) and card I's launch
(ecu.traction.on: a standing start holds the driven tyres at the slip target until the clutch locks, c0f95c2). A
timeline cannot shift by speed, so the upshifts are placed by running it: each pass renders the run, finds where the
engine first reaches 95 % of its redline in the highest gear so far, puts the next upshift there and runs again, until
the car has crossed the line or is in top gear. The times are the engine's of the commit it was made on: make them
again when the engine changes much (a shift a little early or late costs a few hundredths).
"""
import argparse
import csv
import json
import os
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
BUILDS = os.path.join(ROOT, "content", "builds", "archetypes")
OUT = os.path.join(ROOT, "content", "validation", "scenarios", "qm")
GREEN = 10.5


def events(shifts):
    ev = [{"t": 0.0, "ctl": {"mode": 1, "lever": 0, "clutch": 0, "throttle": 0, "brake": 0, "ignition": 1}},
          {"t": 0.0, "spec": {"ecu.traction.on": True}},
          {"t": 0.25, "ctl": {"ignition": 2}}, {"t": 2.0, "ctl": {"ignition": 1}},
          {"t": 5.0, "ctl": {"brake": 1}}, {"t": 5.5, "press": ["shiftUp"]},
          {"t": 8.0, "press": ["strip"]},
          {"t": GREEN, "ctl": {"brake": 0, "throttle": 0.15}}, {"t": GREEN, "ctl": {"throttle": 1}, "ramp": 0.3}]
    ev += [{"t": round(t, 3), "press": ["shiftUp"]} for t in shifts]
    return ev


def scenario(slug, name, shifts):
    return {"format": "tee-scenario", "version": 1, "name": "qm-" + slug,
            "description": "Card P's drag run for \"%s\" in its own gearbox: card I's launch (traction control on, the car's "
                           "own clutch), staged at 8 s, the green at %.1f s, the upshifts where the engine first reaches 95 %% of "
                           "its redline (placed by content/validation/archetypes/quarter_mile.py). Read car.t1320 and "
                           "car.v1320." % (name, GREEN),
            "seconds": 40, "warm": True, "events": events(shifts),
            "assertions": [{"audio": "finite"}, {"audio": "peak", "<=": 1}]}


def render(bin_dir, sim, build, scen_path, out):
    cmd = [os.path.join(bin_dir, "tee-render"), "--build", build, "--scenario", scen_path, "--out", out + ".wav",
           "--tel", "engine.speed,engine.redline,car.gear,car.stripState,car.t1320,car.v1320"]
    if sim:
        cmd += ["--sim", sim]
    subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    rows = list(csv.DictReader(open(out + ".csv")))
    os.remove(out + ".wav")
    return rows


def place(bin_dir, sim, slug, tmp):
    build = os.path.join(BUILDS, slug + ".json")
    b = json.load(open(build))
    gears = b["spec"]["car"]["gears"]
    shifts = []
    path = os.path.join(tmp, slug + ".json")
    for _ in range(gears + 1):
        with open(path, "w") as f:
            json.dump(scenario(slug, b["name"], shifts), f)
        rows = render(bin_dir, sim, build, path, os.path.join(tmp, slug))
        last = rows[-1]
        if len(shifts) >= gears - 1:
            return b["name"], shifts, float(last["car.t1320"]), float(last["car.v1320"])
        after = (shifts[-1] + 0.4) if shifts else GREEN + 0.3
        finish = GREEN + float(last["car.t1320"]) if float(last["car.t1320"]) > 0 else 1e9
        nxt = None
        for r in rows:
            t = float(r["time"])
            if t > finish:
                break
            if t > after and float(r["engine.speed"]) >= 0.95 * float(r["engine.redline"]) > 0:
                nxt = t
                break
        if nxt is None:
            return b["name"], shifts, float(last["car.t1320"]), float(last["car.v1320"])
        shifts.append(nxt)
    return b["name"], shifts, 0.0, 0.0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default=os.path.join(ROOT, "build-dev", "bin"))
    ap.add_argument("--sim", default=None)
    ap.add_argument("--tmp", default=os.path.join(ROOT, "out", "qm"))
    ap.add_argument("slugs", nargs="*")
    a = ap.parse_args()
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(a.tmp, exist_ok=True)
    slugs = a.slugs or sorted(f[:-5] for f in os.listdir(BUILDS) if f.endswith(".json"))
    for slug in slugs:
        b = json.load(open(os.path.join(BUILDS, slug + ".json")))
        if b["spec"]["car"]["gearbox"] in ("Automatic", "CVT") or b["spec"]["car"]["kind"] != "Car":
            continue
        name, shifts, et, trap = place(a.bin, a.sim, slug, a.tmp)
        with open(os.path.join(OUT, slug + ".json"), "w") as f:
            json.dump(scenario(slug, name, shifts), f, indent=2)
            f.write("\n")
        print("%-30s %d shifts  ET %.2f s  trap %.0f km/h" % (slug, len(shifts), et, trap * 3.6), flush=True)


if __name__ == "__main__":
    main()
