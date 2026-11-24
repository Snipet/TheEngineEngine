#!/usr/bin/env python3
"""Bakes each phased archetype's intake cam map, as a maker calibrates one (the lead, 21:05; card C's
`tee-dyno --bake-cam`: the cam swept at full load, the best advance by speed), and writes cam-maps.tsv beside it, which
generate.py reads into ecu.maps.camIn (camInAuto off). A person's own build keeps N1's rule (camInAuto on).

    python3 content/builds/archetypes/cams.py [--bin build-dev/bin] [--sim DIR] [--jobs 2]
Run generate.py first (the bake sweeps the build as it stands, with its runners and headers), then this, then
generate.py again. Run it again when the engine model or a build's hardware changes much. Speeds where the engine
makes no torque (at the limiter) keep the advance of the speed below.
"""
import argparse
import json
import os
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
RPM = 3.14159265358979 / 30.0


def bake(bin_dir, sim, path):
    b = json.load(open(path))
    b["spec"]["ecu"].setdefault("maps", {})
    b["spec"]["ecu"]["maps"].pop("camIn", None)
    b["spec"]["ecu"]["maps"]["camInAuto"] = True
    tmp = os.path.join(ROOT, "out", "cams", os.path.basename(path))
    os.makedirs(os.path.dirname(tmp), exist_ok=True)
    json.dump(b, open(tmp, "w"))
    cmd = [os.path.join(bin_dir, "tee-dyno"), "--bake-cam", "--build", tmp] + (["--sim", sim] if sim else [])
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout
    rows = []
    for line in out.splitlines():
        m = re.match(r"\s*(\d+)\s+(?:-?[\d.]+\s+)+\s*(-?[\d.]+) deg \((-?[\d.]+) N m\)", line)
        if m:
            rows.append((float(m.group(1)) * RPM, float(m.group(2)), float(m.group(3))))
    clean = []
    for w, adv, tq in rows:
        clean.append((w, adv if tq > 0 or not clean else clean[-1][1]))
    return clean


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default=os.path.join(ROOT, "build-dev", "bin"))
    ap.add_argument("--sim", default=None)
    ap.add_argument("--jobs", type=int, default=2)
    a = ap.parse_args()
    paths = sorted(os.path.join(HERE, f) for f in os.listdir(HERE) if f.endswith(".json"))
    phased = [p for p in paths if json.load(open(p))["spec"]["valve"].get("inPhaseRange")]
    with ThreadPoolExecutor(a.jobs) as ex:
        res = dict(zip(phased, ex.map(lambda p: bake(a.bin, a.sim, p), phased)))
    with open(os.path.join(HERE, "cam-maps.tsv"), "w") as f:
        f.write("# build\tspeed rad/s\tintake cam advance, degrees (cams.py: tee-dyno --bake-cam)\n")
        for p in phased:
            slug = os.path.basename(p)[:-5]
            for w, adv in res[p]:
                f.write("%s\t%.1f\t%.0f\n" % (slug, w, adv))
            print("%-30s %s" % (slug, " ".join("%.0f@%.0f" % (adv, w / RPM) for w, adv in res[p])), flush=True)


if __name__ == "__main__":
    main()
