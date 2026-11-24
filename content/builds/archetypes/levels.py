#!/usr/bin/env python3
"""Measures each archetype's free-rev peak with its own listener and writes levels.json beside it: the sound.level
trim (dB) that brings a build under the limiter's stage to a 0.4 peak (L's scheme, 20:27, at 0.4: inside the lead's 0.3 to 0.6, as a 0.3 aim measured 0.24 on first.json; level = 20 log10(0.4 /
peak); a build at the stage's limit, 0.7 and up, keeps 0), capped at +12 dB, the field's range: sound-levels.tsv
beside it (not .json: other tools read every .json here as a build). generate.py reads it.

    python3 content/builds/archetypes/levels.py [--bin build-dev/bin] [--sim DIR]
Run it again when the listener's staging or a build's exhaust changes much, then generate.py.
"""
import argparse
import json
import math
import os
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
AIM, LOUD, MOST = 0.4, 0.7, 12.0


def peak(bin_dir, sim, path, tmp):
    cmd = [os.path.join(bin_dir, "tee-render"), "--build", path, "--scenario",
           os.path.join(ROOT, "content", "validation", "scenarios", "p-free-rev.json"), "--out", tmp + ".wav",
           "--tel", "engine.speed"] + (["--sim", sim] if sim else [])
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    for f in (tmp + ".wav", tmp + ".csv"):
        if os.path.exists(f):
            os.remove(f)
    m = re.search(r"audio\s+peak .*?: ([\d.]+)", out.stdout + out.stderr)
    return float(m.group(1)) if m else None


def trim(p):
    # (only raised: a build already over 0.4 is in the lead's 0.3 to 0.6 and keeps its own loudness)
    return 0.0 if p is None or p >= AIM else round(min(20 * math.log10(AIM / p), MOST), 1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default=os.path.join(ROOT, "build-dev", "bin"))
    ap.add_argument("--sim", default=None)
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--tmp", default=os.path.join(ROOT, "out", "levels"))
    a = ap.parse_args()
    os.makedirs(a.tmp, exist_ok=True)
    paths = sorted(os.path.join(HERE, f) for f in os.listdir(HERE) if f.endswith(".json"))
    paths.append(os.path.join(ROOT, "content", "builds", "first.json"))
    def one(path):
        b = json.load(open(path))
        b["spec"].setdefault("sound", {})["level"] = 0.0
        name = os.path.basename(path)[:-5]
        tmpb = os.path.join(a.tmp, name + ".build.json")
        json.dump(b, open(tmpb, "w"))
        return name, peak(a.bin, a.sim, tmpb, os.path.join(a.tmp, name))
    with ThreadPoolExecutor(a.jobs) as ex:
        res = dict(ex.map(one, paths))
    with open(os.path.join(HERE, "sound-levels.tsv"), "w") as f:
        f.write("# build\tfree-rev peak at level 0\tsound.level dB (levels.py)\n")
        for k, v in sorted(res.items()):
            f.write("%s\t%s\t%+.1f\n" % (k, "none" if v is None else "%.3f" % v, trim(v)))
            print("%-32s peak %s  level %+.1f dB" % (k, v, trim(v)))


if __name__ == "__main__":
    main()
