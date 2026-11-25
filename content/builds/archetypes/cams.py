#!/usr/bin/env python3
"""Bakes each archetype's calibration in the app's own order (the lead, 06:22: one implementation, the app's;
host/src/Bake.cpp, CalibrationBakeService): the knock-limit table (`tee-dyno --bake-spark --save`); on a build with an
intake phaser, the intake cam map on that spark (`--bake-cam --save`, N1's: a load axis, smoothed, the formula's
fallback and its own idle check) and the knock-limit table again on that cam. What the bake owns is kept in
data/bake-maps.json by slug: ecu.maps.camIn and camInKey, ecu.maps.knockLimit and knockLimitKey; generate.py applies
them (the Auto switches off) while BAKED_CAMS is on.

The keys hold the program's sources (Bake.h: knockLimitKey, intakeCamKey): a table baked on one simulator is stale on
any other, and the app bakes it again at launch. So bake on the simulator the content ships with, as the last content
step before `main` moves (the proof, 25 Nov 06:25: a build baked and read on the same tools is Current at once and the
service writes nothing; baked on a simulator one boost file older, it is stale).

    python3 content/builds/archetypes/cams.py --bin DIR --sim DIR [--jobs 3] [slug ...]

Run generate.py first (the bake starts from the build as written), then this, then generate.py again (gated). A build
whose spark bake is refused (a diesel: no knock-limit program) keeps none, and is said so; a cam whose bake fails keeps
the formula.
"""
import argparse
import json
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
KEEP = os.path.join(HERE, "data", "bake-maps.json")
OWNED = ("camIn", "camInKey", "knockLimit", "knockLimitKey")


def dyno(bin_dir, sim, args):
    cmd = [os.path.join(bin_dir, "tee-dyno")] + args + (["--sim", sim] if sim else [])
    return subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)


def bake(bin_dir, sim, path, tmp):
    slug = os.path.basename(path)[:-5]
    b = json.load(open(path))
    maps = b["spec"]["ecu"].setdefault("maps", {})
    for k in OWNED:
        maps.pop(k, None)
    maps["camInAuto"] = True
    maps["knockLimitAuto"] = True
    src = os.path.join(tmp, slug + ".in.json")
    json.dump(b, open(src, "w"))
    log = []
    s1 = os.path.join(tmp, slug + ".spark1.json")
    if os.path.exists(s1):
        os.remove(s1)
    r = dyno(bin_dir, sim, ["--bake-spark", "--build", src, "--save", s1])
    last = s1 if os.path.exists(s1) else src
    log.append("spark: " + ("baked" if os.path.exists(s1) else "none (%s)" % " ".join((r.stdout + r.stderr).strip()
                                                                                         .splitlines()[-1:])))
    if b["spec"]["valve"].get("inPhaseRange"):
        c = os.path.join(tmp, slug + ".cam.json")
        if os.path.exists(c):
            os.remove(c)
        r = dyno(bin_dir, sim, ["--bake-cam", "--build", last, "--save", c])
        idle = [l for l in r.stdout.splitlines() if l.startswith("idle with the map")]
        log.append("cam: " + ("baked" if os.path.exists(c) else "KEPT THE FORMULA") + (" (%s)" % idle[0] if idle else ""))
        if os.path.exists(c):
            last = c
            if os.path.exists(s1):
                s2 = os.path.join(tmp, slug + ".spark2.json")
                if os.path.exists(s2):
                    os.remove(s2)
                dyno(bin_dir, sim, ["--bake-spark", "--build", c, "--save", s2])
                log.append("spark again: " + ("baked" if os.path.exists(s2) else "FAILED (the first kept)"))
                if os.path.exists(s2):
                    last = s2
    saved = json.load(open(last))["spec"]["ecu"].get("maps", {})
    kept = {k: saved[k] for k in OWNED if k in saved and saved[k] not in (None, 0)}
    # a cam map only with its key (an unphased build's saved file carries the empty default); no spark table on a
    # diesel (C's be3ce9c4: the app reads a compression-ignition build as current for it, nothing to bake)
    if "camInKey" not in kept:
        kept.pop("camIn", None)
    if b["spec"]["fuel"].get("kind") == "Diesel":
        kept.pop("knockLimit", None)
        kept.pop("knockLimitKey", None)
    return kept, log


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default=os.path.join(ROOT, "build-dev", "bin"))
    ap.add_argument("--sim", default=None)
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--tmp", default=os.path.join(ROOT, "out", "bake"))
    ap.add_argument("slugs", nargs="*", help="only these (merged into bake-maps.json); default every archetype")
    a = ap.parse_args()
    os.makedirs(a.tmp, exist_ok=True)
    paths = sorted(os.path.join(HERE, f) for f in os.listdir(HERE) if f.endswith(".json")
                   and (not a.slugs or f[:-5] in a.slugs))
    with ThreadPoolExecutor(a.jobs) as ex:
        res = dict(zip(paths, ex.map(lambda p: bake(a.bin, a.sim, p, a.tmp), paths)))
    maps = json.load(open(KEEP)) if a.slugs and os.path.exists(KEEP) else {}
    for p in paths:
        slug = os.path.basename(p)[:-5]
        kept, lines = res[p]
        maps.pop(slug, None)
        if kept:
            maps[slug] = kept
        print("%-30s %-28s %s" % (slug, ", ".join(sorted(kept)) or "nothing baked", "; ".join(lines)), flush=True)
    with open(KEEP, "w") as f:
        json.dump(maps, f, indent=1)
        f.write("\n")


if __name__ == "__main__":
    main()
