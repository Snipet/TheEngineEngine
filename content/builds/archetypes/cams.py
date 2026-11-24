#!/usr/bin/env python3
"""Bakes each phased archetype's intake cam map as a maker calibrates one (the lead, 21:05): N1's
`tee-dyno --bake-cam --build B --save FILE` (2b04859c: a load axis, parked up to 0.75 of the air's pressure and the
baked advance from 0.95, smoothed across speed, the formula's phase wherever the map would lose over 2 % at full load,
and the file written only if it idles, warm and cold, as generate.py's gate asks), then keeps each saved map in
cam-maps.json beside it, which generate.py applies to ecu.maps.camIn (camInAuto off) only while BAKED_CAMS is on.

    python3 content/builds/archetypes/cams.py [--bin build-dev/bin] [--sim DIR] [--jobs 3]
Run generate.py first (the bake sweeps the build as it stands), then this, then generate.py again. A build whose bake
fails its idle keeps the formula (it is left out of cam-maps.json, and said so).
"""
import argparse
import json
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))


def bake(bin_dir, sim, path, tmp):
    b = json.load(open(path))
    maps = b["spec"]["ecu"].setdefault("maps", {})
    maps.pop("camIn", None)
    maps["camInAuto"] = True
    src = os.path.join(tmp, "in-" + os.path.basename(path))
    out = os.path.join(tmp, "out-" + os.path.basename(path))
    json.dump(b, open(src, "w"))
    if os.path.exists(out):
        os.remove(out)
    cmd = [os.path.join(bin_dir, "tee-dyno"), "--bake-cam", "--build", src, "--save", out] + (["--sim", sim] if sim else [])
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    if r.returncode != 0 or not os.path.exists(out):
        return None, (r.stdout + r.stderr).strip().splitlines()[-3:]
    saved = json.load(open(out))["spec"]["ecu"]["maps"]
    idle = [l for l in r.stdout.splitlines() if l.startswith("idle with the map")]
    return saved.get("camIn"), idle


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default=os.path.join(ROOT, "build-dev", "bin"))
    ap.add_argument("--sim", default=None)
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--tmp", default=os.path.join(ROOT, "out", "cams"))
    a = ap.parse_args()
    os.makedirs(a.tmp, exist_ok=True)
    paths = sorted(os.path.join(HERE, f) for f in os.listdir(HERE) if f.endswith(".json") and f != "cam-maps.json")
    phased = [p for p in paths if json.load(open(p))["spec"]["valve"].get("inPhaseRange")]
    with ThreadPoolExecutor(a.jobs) as ex:
        res = dict(zip(phased, ex.map(lambda p: bake(a.bin, a.sim, p, a.tmp), phased)))
    maps = {}
    for p in phased:
        slug = os.path.basename(p)[:-5]
        m, lines = res[p]
        if m:
            maps[slug] = m
        print("%-30s %s  %s" % (slug, "baked" if m else "KEPT THE FORMULA", " ".join(lines)), flush=True)
    with open(os.path.join(HERE, "cam-maps.json"), "w") as f:
        json.dump(maps, f, indent=1)
        f.write("\n")


if __name__ == "__main__":
    main()
