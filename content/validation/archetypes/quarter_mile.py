#!/usr/bin/env python3
"""Writes a drag-run scenario for each archetype whose gearbox is shifted by the driver (manual on the paddles,
sequential, dual clutch), and for each automatic: content/validation/scenarios/qm/<slug>.json, and points its
validation's quarter-mile targets at it.

    python3 content/validation/archetypes/quarter_mile.py [--bin build-dev/bin] [--sim DIR] [--builds DIR]
        [--shift power|redline] [--launch idle|prerev] [--tyres air|warm] [--out DIR] [slug ...]

A timed run as it is made (the lead, 19:36), each part a switch so the table's notes can say what each one gives:
  --shift power    each upshift where the next gear's wheel torque at the same road speed is the greater, from the
                   engine's predicted full-load curve (tee-dyno --predict); at 97 % of the redline where the next gear
                   is never the greater before it. (redline: at 95 % of the redline, as before.)
  --launch prerev  the revs held at the green: the brake held and the pedal floored 1.5 s before it (the paddles'
                   and the dual clutch's own clutch stays open while the brake is held: a pre-rev against the clutch),
                   with the launch button held where the car has launch control (ecu.launch.on), so it holds its
                   launch speed; an automatic's is a brake-torque launch at a fifth of the pedal for a second (where its brakes hold
                   against the converter: card I's), then the pedal to the floor over 0.6 s. (idle: the pedal floored
                   at the green from idle, as before.)
  --tyres warm     the tyres in their compound's window, as a timed run makes them (the scenario's "warmTyres":
                   true, card C's: the host holds car.m1.warmTyres, card I's a140c6f). (air: at the air's temperature,
                   as every other run starts them; the default until card C's field lands: car.m1.warmTyres is state,
                   which a scenario's spec event cannot set.)

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
import re
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
BUILDS = os.path.join(ROOT, "content", "builds", "archetypes")
OUT = os.path.join(ROOT, "content", "validation", "scenarios", "qm")
RPM = 3.14159265358979 / 30.0


class Run:
    """How the run is made: the switches above, and what the build has (its gearbox, drive, launch control)."""

    def __init__(self, shift="power", launch="prerev", tyres="air"):
        self.shift, self.launch, self.tyres = shift, launch, tyres

    def for_build(self, b):
        c, e = b["spec"]["car"], b["spec"].get("ecu", {})
        self.auto = c["gearbox"] in ("Automatic", "CVT")
        self.button = bool(e.get("launch", {}).get("on")) if isinstance(e.get("launch"), dict) else bool(e.get("launch.on"))
        self.stage = 8.0
        self.green = self.stage + 2.5
        return self


def events(run, shifts):
    g = run.green
    ev = [{"t": 0.0, "ctl": {"mode": 1, "lever": 0, "clutch": 0, "throttle": 0, "brake": 0, "ignition": 1}},
          {"t": 0.25, "ctl": {"ignition": 2}}, {"t": 2.0, "ctl": {"ignition": 1}},
          {"t": 5.0, "ctl": {"brake": 1}}]
    # first on the paddles; an automatic's lever in D (it keeps its own launch: no traction control)
    ev += [{"t": 5.5, "ctl": {"lever": 1}}] if run.auto else [{"t": 5.5, "press": ["shiftUp"]}]
    if not run.auto:
        ev += [{"t": 0.0, "spec": {"ecu.traction.on": True}}]
    ev += [{"t": run.stage, "press": ["strip"]}]
    if run.launch == "prerev" and run.auto:
        # an automatic's brake-torque launch as a driver makes it (card I's launch.automatic): the brakes held and the
        # pedal at a fifth for a second, where the rear brakes still hold against the converter's stall (at a full
        # pedal they do not: a burnout before the green), then the brakes off and the pedal to the floor over 0.6 s
        ev += [{"t": g - 1.0, "ctl": {"throttle": 0.2}}, {"t": g, "ctl": {"brake": 0}},
               {"t": g, "ctl": {"throttle": 1}, "ramp": 0.6}]
    elif run.launch == "prerev":
        ev += [{"t": g - 1.5, "ctl": {"throttle": 1}, "ramp": 0.3}, {"t": g, "ctl": {"brake": 0}}]
        if run.button:
            ev += [{"t": g - 1.6, "ctl": {"buttons": 2}}, {"t": g + 1.0, "ctl": {"buttons": 0}}]
    else:
        ev += [{"t": g, "ctl": {"brake": 0, "throttle": 0.15}},
               {"t": g, "ctl": {"throttle": 1}, "ramp": 0.6 if run.auto else 0.3}]
    ev += [{"t": round(t, 3), "press": ["shiftUp"]} for t in shifts]
    return sorted(ev, key=lambda x: x["t"])


def shift_speeds(bin_dir, sim, build_path, b):
    """The engine speed (rad/s) of each upshift, gear by gear: where the next gear's wheel torque at the same road
    speed is the greater, from the predicted full-load curve; 97 % of the redline where it never is before it."""
    c, e = b["spec"]["car"], b["spec"]["engine"]
    red = e["redlineSpeed"]
    cmd = [os.path.join(bin_dir, "tee-dyno"), "--predict", "--points", "48", "--from", str(round(1000 * RPM, 3)),
           "--to", str(round(red, 3)), "--build", build_path] + (["--sim", sim] if sim else [])
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout
    pts = sorted((float(m.group(1)) * RPM, float(m.group(2))) for m in
                 (re.match(r"\s+(\d+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+\d+\s+\w+", l) for l in out.splitlines()) if m)

    def torque(w):
        if not pts or w <= pts[0][0]:
            return pts[0][1] if pts else 0.0
        for (w0, t0), (w1, t1) in zip(pts, pts[1:]):
            if w <= w1:
                return t0 + (t1 - t0) * (w - w0) / (w1 - w0)
        return pts[-1][1]

    ratios = c["ratio"]
    peak = max(pts, key=lambda p: p[1])[0] if pts else 0.0
    speeds = []
    for g0, g1 in zip(ratios, ratios[1:]):
        at = 0.97 * red
        w = peak
        while w < 0.97 * red:
            if torque(w * g1 / g0) * g1 >= torque(w) * g0:
                at = w
                break
            w += 2.0
        speeds.append(at)
    return speeds, pts


def scenario(run, slug, name, shifts):
    how = [("its own gearbox's shifts" if run.auto else
            "the upshifts where the next gear's wheel torque is the greater (from the predicted curve)"
            if run.shift == "power" else "the upshifts where the engine first reaches 95 % of its redline"),
           ("the revs held at the green (the brake held, the pedal floored 1.5 s before%s)" %
            (", the launch button held" if run.button else "")) if run.launch == "prerev" else
           "the pedal floored at the green from idle"]
    how.append("the tyres warm, in their window" if run.tyres == "warm" else "the tyres at the air's temperature")
    doc = {"format": "tee-scenario", "version": 1, "name": "qm-" + slug,
            "description": "Card P's drag run for \"%s\": card I's launch (traction control on, the car's own clutch), "
                           "staged at %.1f s, the green at %.1f s; %s (placed by content/validation/archetypes/"
                           "quarter_mile.py). Read car.t1320 and car.v1320." % (name, run.stage, run.green, "; ".join(how)),
            "seconds": 50, "warm": True, "events": events(run, shifts),
            "assertions": [{"audio": "finite"}, {"audio": "peak", "<=": 1}]}
    if run.tyres == "warm":
        doc["warmTyres"] = True
    return doc


def render(bin_dir, sim, build, scen_path, out):
    cmd = [os.path.join(bin_dir, "tee-render"), "--build", build, "--scenario", scen_path, "--out", out + ".wav",
           "--tel", "engine.speed,engine.redline,car.gear,car.stripState,car.t1320,car.v1320"]
    if sim:
        cmd += ["--sim", sim]
    subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    rows = list(csv.DictReader(open(out + ".csv")))
    os.remove(out + ".wav")
    return rows


def place(bin_dir, sim, builds, slug, tmp, run):
    build = os.path.join(builds, slug + ".json")
    b = json.load(open(build))
    run.for_build(b)
    gears = b["spec"]["car"]["gears"]
    at = shift_speeds(bin_dir, sim, build, b)[0] if run.shift == "power" and not run.auto else None
    shifts = []
    path = os.path.join(tmp, slug + ".json")
    for _ in range(gears + 1):
        with open(path, "w") as f:
            json.dump(scenario(run, slug, b["name"], shifts), f)
        rows = render(bin_dir, sim, build, path, os.path.join(tmp, slug))
        last = rows[-1]
        if run.auto or len(shifts) >= gears - 1:
            return b["name"], shifts, float(last["car.t1320"]), float(last["car.v1320"])
        after = (shifts[-1] + 0.4) if shifts else run.green + 0.3
        finish = run.green + float(last["car.t1320"]) if float(last["car.t1320"]) > 0 else 1e9
        nxt = None
        for r in rows:
            t = float(r["time"])
            if t > finish:
                break
            want = at[len(shifts)] if at else 0.95 * float(r["engine.redline"])
            if t > after and int(float(r["car.gear"])) == len(shifts) + 1 and float(r["engine.speed"]) >= want > 0:
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
    ap.add_argument("--builds", default=BUILDS)
    ap.add_argument("--out", default=OUT)
    ap.add_argument("--shift", choices=("power", "redline"), default="power")
    ap.add_argument("--launch", choices=("prerev", "idle"), default="prerev")
    ap.add_argument("--tyres", choices=("air", "warm"), default="air")
    ap.add_argument("slugs", nargs="*")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    os.makedirs(a.tmp, exist_ok=True)
    slugs = a.slugs or sorted(f[:-5] for f in os.listdir(a.builds) if f.endswith(".json"))
    for slug in slugs:
        b = json.load(open(os.path.join(a.builds, slug + ".json")))
        if b["spec"]["car"]["kind"] != "Car" or b["spec"]["car"]["gearbox"] == "CVT":
            continue
        run = Run(a.shift, a.launch, a.tyres)
        name, shifts, et, trap = place(a.bin, a.sim, a.builds, slug, a.tmp, run)
        with open(os.path.join(a.out, slug + ".json"), "w") as f:
            json.dump(scenario(run, slug, name, shifts), f, indent=2)
            f.write("\n")
        print("%-30s %d shifts  ET %.2f s  trap %.0f km/h" % (slug, len(shifts), et, trap * 3.6), flush=True)


if __name__ == "__main__":
    main()
