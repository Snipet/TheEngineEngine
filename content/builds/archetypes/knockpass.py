#!/usr/bin/env python3
"""Table L's second pass (the lead, 04:10): C's knock-limit table on the naturally aspirated archetypes, baked in the
app's own order (CalibrationBakeService: the spark map; the intake cam on that spark, where the build has a phaser; the
spark map again), and each build measured baked against its cams-only form (as committed):

  - on the engine dyno at 0.35, 0.5, 0.65, 0.8 and 0.95 of its redline (C's proof's points: knock-table-proof.md), held
    at full throttle from 2 s, read as the means from 8 to 10 s: the torque, the knock retard and the spark asked;
  - at warm idle (p-idle-warm, 8 to 14 s): its speed, the speed's spread and the manifold.

C's proof's conditions (the lead's): the knock retard under 3 deg in steady state; the torque within 2 %; no more advance
asked than before. A point the table moves by more than 0.5 % (N1's floor for one predicted point) is marked too.

    python3 content/builds/archetypes/knockpass.py --bin DIR --sim DIR --out DIR [--jobs 3] [slug ...]

Writes OUT/<slug>.baked.json (the build with its tables) and OUT/knockpass.md (the table). Nothing in the repository is
changed: the baked builds are applied by the generator only once the lead has read the table.
"""
import argparse
import json
import math
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
RPM = math.pi / 30.0
FRACTIONS = (0.35, 0.5, 0.65, 0.8, 0.95)


def dyno(bin_dir, sim, args):
    cmd = [os.path.join(bin_dir, "tee-dyno")] + args + (["--sim", sim] if sim else [])
    return subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)


def bake(bin_dir, sim, slug, src, out):
    """The app's order: spark, cam on that spark (a phased build), spark again. Returns (path, log lines)."""
    log = []
    s1 = os.path.join(out, slug + ".spark1.json")
    r = dyno(bin_dir, sim, ["--bake-spark", "--build", src, "--save", s1])
    log.append("spark: %s" % ("ok" if os.path.exists(s1) else (r.stdout + r.stderr).strip().splitlines()[-1:]))
    if not os.path.exists(s1):
        return None, log
    last = s1
    b = json.load(open(s1))
    if b["spec"]["valve"].get("inPhaseRange", 0) > 0:
        c = os.path.join(out, slug + ".cam.json")
        r = dyno(bin_dir, sim, ["--bake-cam", "--build", s1, "--save", c])
        idle = [l for l in r.stdout.splitlines() if l.startswith("idle with the map")]
        log.append("cam: %s %s" % ("ok" if os.path.exists(c) else "kept the spark's build (no map saved)",
                                   idle[0] if idle else ""))
        if os.path.exists(c):
            s2 = os.path.join(out, slug + ".spark2.json")
            r = dyno(bin_dir, sim, ["--bake-spark", "--build", c, "--save", s2])
            log.append("spark again: %s" % ("ok" if os.path.exists(s2) else "failed"))
            last = s2 if os.path.exists(s2) else c
    final = os.path.join(out, slug + ".baked.json")
    json.dump(json.load(open(last)), open(final, "w"), indent=2)
    return final, log


def scenario(out, rpm):
    path = os.path.join(out, "hold-%d.json" % rpm)
    if not os.path.exists(path):
        json.dump({"format": "tee-scenario", "version": 1, "name": "hold-%d" % rpm,
                   "description": "knockpass: the engine dyno held at %d rpm, warm, full throttle from 2 s" % rpm,
                   "seconds": 10, "warm": True,
                   "events": [{"t": 0.0, "ctl": {"mode": 2, "dynoSpeed": round(rpm * RPM, 2), "dynoRate": 0,
                                                 "ignition": 1}},
                              {"t": 0.25, "ctl": {"ignition": 2}}, {"t": 1.5, "ctl": {"ignition": 1}},
                              {"t": 2.0, "ctl": {"throttle": 1}, "ramp": 0.2}]}, open(path, "w"))
    return path


def render(bin_dir, sim, build, scen, tels, t0, t1, tag, out):
    wav = os.path.join(out, tag + ".wav")
    cmd = [os.path.join(bin_dir, "tee-render"), "--build", build, "--scenario", scen, "--out", wav,
           "--tel", ",".join(tels)] + (["--sim", sim] if sim else [])
    subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    csv = wav[:-4] + ".csv"
    rows = []
    if os.path.exists(csv):
        with open(csv) as f:
            head = next(f).strip().split(",")
            rows = [list(map(float, l.split(","))) for l in f if l.strip()]
    for x in (wav, csv):
        if os.path.exists(x):
            os.remove(x)
    rows = [r for r in rows if t0 <= r[0] <= t1]
    if not rows:
        return None
    mean = {h: sum(r[i] for r in rows) / len(rows) for i, h in enumerate(head)}
    sd = {h: math.sqrt(sum((r[i] - mean[h]) ** 2 for r in rows) / len(rows)) for i, h in enumerate(head)}
    return mean, sd


def measure(bin_dir, sim, build, slug, form, out):
    b = json.load(open(build))
    red = b["spec"]["engine"]["redlineSpeed"] / RPM
    pts = []
    for f in FRACTIONS:
        rpm = int(round(red * f / 50.0) * 50)
        m = render(bin_dir, sim, build, scenario(out, rpm), ["engine.torque", "ecu.knockRetard", "ecu.advanceAsked"],
                   8.0, 10.0, "%s-%s-%d" % (slug, form, rpm), out)
        pts.append((rpm, m[0] if m else None))
    idle = render(bin_dir, sim, build, os.path.join(ROOT, "content", "validation", "scenarios", "p-idle-warm.json"),
                  ["engine.speed", "intake.manifold"], 8.0, 14.0, "%s-%s-idle" % (slug, form), out)
    return pts, idle


def one(a, slug):
    src = os.path.join(ROOT, "content", "builds", "archetypes", slug + ".json")
    baked, log = bake(a.bin, a.sim, slug, src, a.out)
    off = measure(a.bin, a.sim, src, slug, "off", a.out)
    on = measure(a.bin, a.sim, baked, slug, "on", a.out) if baked else None
    return slug, log, off, on


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--bin", default=os.path.join(ROOT, "build-dev", "bin"))
    p.add_argument("--sim", default=None)
    p.add_argument("--out", required=True)
    p.add_argument("--jobs", type=int, default=3)
    p.add_argument("slugs", nargs="*")
    a = p.parse_args()
    os.makedirs(a.out, exist_ok=True)
    slugs = a.slugs or sorted(
        f[:-5] for f in os.listdir(HERE) if f.endswith(".json")
        and json.load(open(os.path.join(HERE, f)))["spec"]["boost"]["kind"] == "Natural"
        and json.load(open(os.path.join(HERE, f)))["spec"]["fuel"].get("kind") != "Diesel")
    with ThreadPoolExecutor(a.jobs) as ex:
        results = list(ex.map(lambda s: one(a, s), slugs))
    out = ["| archetype | rpm | torque off / on N m | change % | retard off / on | ask off / on | marks |",
           "|---|---|---|---|---|---|---|"]
    idle = ["| archetype | idle off: rpm, sd, kPa | idle on: rpm, sd, kPa | bake |", "|---|---|---|---|"]
    for slug, log, off, on in results:
        for i, (rpm, m0) in enumerate(off[0]):
            m1 = on[0][i][1] if on else None
            if not m0 or not m1:
                out.append("| %s | %d | did not run | | | | |" % (slug, rpm))
                continue
            t0, t1 = m0["engine.torque"], m1["engine.torque"]
            ch = 100.0 * (t1 / t0 - 1.0) if t0 else 0.0
            marks = []
            if m1["ecu.knockRetard"] >= 3.0:
                marks.append("retard >= 3")
            if abs(ch) > 2.0:
                marks.append("torque > 2 %")
            if m1["ecu.advanceAsked"] > m0["ecu.advanceAsked"] + 0.1:
                marks.append("more advance asked")
            if abs(ch) > 0.5:
                marks.append("> 0.5 %")
            out.append("| %s | %d | %.1f / %.1f | %+.1f | %.1f / %.1f | %.1f / %.1f | %s |" % (
                slug, rpm, t0, t1, ch, m0["ecu.knockRetard"], m1["ecu.knockRetard"], m0["ecu.advanceAsked"],
                m1["ecu.advanceAsked"], ", ".join(marks) or ""))

        def show(i):
            if not i:
                return "did not run"
            return "%.0f, %.1f, %.1f" % (i[0]["engine.speed"] / RPM, i[1]["engine.speed"] / RPM,
                                         i[0]["intake.manifold"] / 1000.0)
        idle.append("| %s | %s | %s | %s |" % (slug, show(off[1]), show(on[1] if on else None), "; ".join(map(str, log))))
    open(os.path.join(a.out, "knockpass.md"), "w").write("\n".join(out + [""] + idle) + "\n")
    print("\n".join(out + [""] + idle))


if __name__ == "__main__":
    main()
