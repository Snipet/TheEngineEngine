#!/usr/bin/env python3
"""Card P's misses table: runs (or reads) tee-validate on every archetype and writes docs/manual/validation.md.

    python3 content/validation/archetypes/report.py [--bin build-dev/bin] [--from DIR] [--jobs 3]

--from DIR reads tee-validate's printed tables from DIR/<slug>.txt instead of running it (each run takes one to
three minutes: the top-speed run is 115 s of driving).
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
RPM = math.pi / 30.0
ROW = re.compile(r"\s(\S+)\s+(pass|FAIL|MISS|n/a)(\s|$)")


CONTENT = None
SIM = None


def run(bin_dir, slug, out_dir):
    path = os.path.join(out_dir, slug + ".txt")
    if not os.path.exists(path):
        cmd = [os.path.join(bin_dir, "tee-validate"), "--simulator", os.path.join(HERE, slug + ".json")]
        if CONTENT:
            cmd += ["--content", CONTENT]
        if SIM:
            cmd += ["--sim", SIM]
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        with open(path, "w") as f:
            f.write(r.stdout + r.stderr)
    with open(path) as f:
        return f.read()


TONE = {"name": "Idle: no standing tone", "range": [-60, -10], "unit": "dB", "basis": "judgement",
        "source": "the lead's ruling (21:57, the owner heard the first build hum: the in-tank pump at 3.9 kHz, the fan "
                  "at 280 Hz): at warm idle (8 to 14 s) the strongest line above 300 Hz (within 0.2 Hz), A-weighted, "
                  "stands at least 10 dB under the exhaust's whole band from 20 to 300 Hz, A-weighted (judgement: a "
                  "tone within 10 dB of the engine's own note is heard as a hum); measured by report.py on a "
                  "tee-render of p-idle-warm"}


COMB = {"name": "Idle: the clatter's comb (diesel)", "range": [0, 8], "unit": "dB", "basis": "recalled",
        "source": "the lead's ruling (00:33, D2's measure): a turbo diesel's idle is its clatter over a muffled exhaust "
                  "(the vanes closed at idle), so its row is the clatter's prominence, not a tone against the exhaust: at "
                  "warm idle (8 to 14 s) a Welch spectrum (2^16-sample Hann, half overlap), the firing frequency f found "
                  "within 3 %% of the telemetry's, then 10 log10 of the summed peaks within 1.5 Hz of h f over the mean "
                  "within 1.5 Hz of (h + 1/2) f, for the harmonics from 1 to 3 kHz; at most 8 dB, the tone-to-noise "
                  "prominence threshold above 1 kHz of ECMA-74 Annex D (recalled by D2, not read)"}


CROSS = {"name": "Stages: where the high cam passes the low (rpm)", "range": [5500, 5900], "unit": "rpm",
         "basis": "recalled",
         "source": "the lead's ruling (02:17): on a stock engine of the kind the two stages' full-load torque curves cross "
                   "within some 300 rpm of the factory switch: Hondata's tuning help (hondata.com, a tuner's, looked up: "
                   "\"Set the VTEC point to the intersection of the high speed cam and the low speed cam\"; \"stock "
                   "engine 5500-5700 rpm\") and an S2000 owner's \"torque curve intersection to 5800-5900\" (S2KI, a "
                   "forum, a search snippet); measured by report.py: tee-dyno --predict from 3000 to 8500 rpm every 250 "
                   "with each stage held (valve.stageSpeed 1100 rad/s: the low stage throughout; 100: the high), the "
                   "first speed where the high stage's torque passes the low's, interpolated; past 8500 reads 8500. "
                   "NOTE: the build's low stage is judgement (no source) and its high lobe an aggregator's, so a miss "
                   "may be the card's as much as the model's"}
CROSS_SLUGS = ("two-stage-cam-20",)


def crossing(bin_dir, slug, out_dir, build_path):
    """CROSS: where the held high stage's full-load torque first passes the held low stage's, rpm (cached)."""
    b = json.load(open(build_path))
    curves = {}
    for tag, sp in (("low", 1100.0), ("high", 100.0)):
        path = os.path.join(out_dir, "%s.stage-%s.txt" % (slug, tag))
        if not os.path.exists(path):
            bb = json.loads(json.dumps(b))
            bb["spec"]["valve"]["stageSpeed"] = sp
            src = os.path.join(out_dir, "%s.stage-%s.json" % (slug, tag))
            json.dump(bb, open(src, "w"))
            cmd = [os.path.join(bin_dir, "tee-dyno"), "--predict", "--points", "23", "--from", str(3000 * RPM),
                   "--to", str(8500 * RPM), "--build", src] + (["--sim", SIM] if SIM else [])
            r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
            open(path, "w").write(r.stdout)
        pts = []
        for line in open(path):
            m = re.match(r"\s+(\d+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+\d+\s+\w+", line)
            if m:
                pts.append((float(m.group(1)), float(m.group(2))))
        curves[tag] = sorted(pts)
    lo, hi = curves["low"], curves["high"]
    if not lo or len(lo) != len(hi):
        return None
    d = [h[1] - l[1] for l, h in zip(lo, hi)]
    for i in range(1, len(d)):
        if d[i - 1] < 0 <= d[i]:
            f = -d[i - 1] / (d[i] - d[i - 1])
            return lo[i - 1][0] + f * (lo[i][0] - lo[i - 1][0])
    return lo[0][0] if d[0] >= 0 else lo[-1][0]


TAKEUP = {"name": "Take-up: 90 % of peak torque after the pedal, at a lug (s)", "range": [0, 5], "unit": "s",
          "basis": "cited",
          "source": "the lead's ruling (03:11, D2's reading 03:13): held at 1100 rpm on the dyno (a loaded truck's speed "
                    "barely moves in the seconds the makers quote), the pedal floored from none at 6 s, the time to 90 %% "
                    "of the build's own peak torque (its predicted curve's top). The makers' claims: Detroit's DD15 spec "
                    "sheet \"90%% peak torque in 1.5 seconds\", \"At just over 1100 RPM\"; the older engines' \"four "
                    "seconds\" (the S60, TRUCKiNG Info 2007) and competitors' \"up to 4.4 seconds\" (Detroit's "
                    "brochure); 5 s their slowest with margin (judgement). Not reached in the 34 s read: 34. Printed "
                    "beside it: the time to 90 %% of what it settles at, and the settled torque over the peak"}
TAKEUP_SLUGS = ("tractor-15-inline-six",)


def takeup(bin_dir, slug, out_dir, build_path, peak):
    """TAKEUP: seconds from the pedal to 90 % of `peak` (N m), and to 90 % of the settled torque, and settled / peak."""
    path = os.path.join(out_dir, slug + ".takeup.txt")
    if not os.path.exists(path):
        wav = os.path.join(out_dir, slug + ".takeup.wav")
        cmd = [os.path.join(bin_dir, "tee-render"), "--build", build_path, "--scenario",
               os.path.join(ROOT, "content", "validation", "scenarios", "p-takeup-1100.json"), "--out", wav,
               "--tel", "engine.torque"] + (["--sim", SIM] if SIM else [])
        subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        rows = []
        with open(wav[:-4] + ".csv") as f:
            next(f)
            for line in f:
                t, q = line.split(",")[:2]
                rows.append((float(t), float(q)))
        for x in (wav, wav[:-4] + ".csv"):
            if os.path.exists(x):
                os.remove(x)
        end = [q for t, q in rows if t >= 36.0]
        settled = sum(end) / max(len(end), 1)
        after = [(t - 6.0, q) for t, q in rows if t >= 6.0]
        t_peak = next((t for t, q in after if q >= 0.9 * peak), 34.0)
        t_set = next((t for t, q in after if q >= 0.9 * settled), 34.0)
        with open(path, "w") as fh:
            fh.write("%.2f %.2f %.1f\n" % (t_peak, t_set, settled))
    v = open(path).read().split()
    return float(v[0]), float(v[1]), float(v[2])


def comb_depth(x, sr, fire):
    """D2's comb depth (dB): the firing harmonics from 1 to 3 kHz against the half-way gaps, Welch, A-weighting
    cancelling in the ratio."""
    import numpy as np
    n = 1 << 16
    hop = n // 2
    w = np.hanning(n)
    segs = [x[i:i + n] * w for i in range(0, len(x) - n + 1, hop)]
    if not segs:
        return None
    pw = np.mean([np.abs(np.fft.rfft(g)) ** 2 for g in segs], axis=0)
    f = np.fft.rfftfreq(n, 1.0 / sr)

    def depth(f0):
        top = gap = 0.0
        h = int(np.ceil(1000.0 / f0))
        while h * f0 <= 3000.0:
            a = (f > h * f0 - 1.5) & (f < h * f0 + 1.5)
            b = (f > (h + 0.5) * f0 - 1.5) & (f < (h + 0.5) * f0 + 1.5)
            if a.any() and b.any():
                top += pw[a].max()
                gap += pw[b].mean()
            h += 1
        return top, gap
    best = max((depth(f0) + (f0,) for f0 in np.linspace(fire * 0.97, fire * 1.03, 121)), key=lambda r: r[0])
    return 10 * np.log10(best[0] / best[1]) if best[1] > 0 else None


def tone(bin_dir, slug, out_dir, build_path):
    """The idle's strongest line above 300 Hz against its exhaust band, dB, A-weighted (TONE), cached in out_dir."""
    import struct
    import numpy as np
    path = os.path.join(out_dir, slug + ".tone.txt")
    if not os.path.exists(path):
        wav = os.path.join(out_dir, slug + ".idle.wav")
        cmd = [os.path.join(bin_dir, "tee-render"), "--build", build_path, "--scenario",
               os.path.join(ROOT, "content", "validation", "scenarios", "p-idle-warm.json"), "--out", wav,
               "--tel", "engine.speed"] + (["--sim", SIM] if SIM else [])
        subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        raw = open(wav, "rb").read()
        i = raw.find(b"data")
        n = struct.unpack("<I", raw[i + 4:i + 8])[0]
        x = np.frombuffer(raw[i + 8:i + 8 + n], dtype="<f4").reshape(-1, 2).mean(1)
        sr = 48000
        seg = x[8 * sr:14 * sr]
        pw = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        f = np.maximum(np.fft.rfftfreq(len(seg), 1.0 / sr), 1.0)
        f2 = f ** 2
        ra = 12194.0 ** 2 * f2 ** 2 / ((f2 + 20.6 ** 2) * np.sqrt((f2 + 107.7 ** 2) * (f2 + 737.9 ** 2)) * (f2 + 12194.0 ** 2))
        pw = pw * (ra / 0.7943) ** 2
        k = int(np.argmax(pw * (f > 300)))
        line = pw[max(k - 1, 0):k + 2].sum()
        band = pw[(f > 20) & (f < 300)].sum()
        b = json.load(open(build_path))
        e = b["spec"]["engine"]
        cyl = e.get("cylinders") or 2 * e.get("rotors", 2)
        fire = b["spec"]["ecu"]["idle"]["speed"] * 30 / np.pi / 60.0 * cyl / 2.0
        cd = comb_depth(x[8 * sr:14 * sr], sr, fire) if b["spec"]["fuel"].get("kind") == "Diesel" else None
        with open(path, "w") as fh:
            fh.write("%.2f %.1f %s\n" % (10 * np.log10(line / band), f[k], "%.2f" % cd if cd is not None else "-"))
        os.remove(wav)
        for extra in (wav[:-4] + ".csv",):
            if os.path.exists(extra):
                os.remove(extra)
    v = open(path).read().split()
    return float(v[0]), float(v[1]), (float(v[2]) if len(v) > 2 and v[2] != "-" else None)


def basis(t):
    """A target's kind of source: cited, recalled or judgement (judgement where its file says none, as the curve's
    sharpest point, the lead's ruling)."""
    return t.get("basis", "judgement")


def parse(text, targets):
    lines = text.splitlines()
    try:
        start = next(i for i, l in enumerate(lines) if l.startswith("target ")) + 1
    except StopIteration:
        return None
    rows = []
    for t, line in zip(targets, lines[start:start + len(targets)]):
        m = ROW.search(line)
        if not m:
            rows.append((t, None, "n/a"))
            continue
        try:
            val = float(m.group(1))
        except ValueError:
            val = None
        st = "FAIL" if m.group(2) == "MISS" else m.group(2)
        # judged again against the range in the file now (tee-validate's own rule), so a band moved on its source
        # counts when an earlier run's output is re-read
        if val is not None and st in ("pass", "FAIL") and "range" in t:
            st = "pass" if min(t["range"]) <= val <= max(t["range"]) else "FAIL"
        rows.append((t, val, st))
    return rows


def show(t, v):
    """A measured or target value in a person's units."""
    if v is None:
        return "-"
    unit = t.get("unit", "")
    if unit == "W":
        return "%.0f kW" % (v / 1000)
    if unit == "rad/s":
        return "%.0f rpm" % (v / RPM)
    if unit.startswith("Pa"):
        return "%.2f bar" % (v / 1e5)
    if unit == "m/s" and "speed" in t["name"].lower() or t["name"].startswith("Quarter mile: trap"):
        return "%.0f km/h" % (v * 3.6)
    if unit == "m/s":
        return "%.1f m/s" % v
    if unit == "s":
        return "%.2f s" % v
    if unit == "K":
        return "%.0f K" % v
    if unit == "N m":
        return "%.0f N m" % v
    if unit == "%":
        return "%.1f %%" % v
    if unit == "fraction":
        return "%.2f" % v
    return "%.3g" % v


def rng(t):
    a, b = t["range"]
    return "%s to %s" % (show(t, a), show(t, b))


def curve(bin_dir, slug, out_dir, build_path, plan):
    """The predicted curve at tee-validate's own spacing (Validation.cpp: one point every 15 rad/s, 8 to 48 points),
    from tee-dyno --predict, cached as out_dir/<slug>.curve.csv: [(rad/s, N m, W)]."""
    path = os.path.join(out_dir, slug + ".curve.txt")
    if not os.path.exists(path):
        n = min(max(int((plan["to"] - plan["from"]) / 15.0) + 1, 8), 48)
        cmd = [os.path.join(bin_dir, "tee-dyno"), "--predict", "--points", str(n), "--from", str(plan["from"]),
               "--to", str(plan["to"]), "--build", build_path]
        if SIM:
            cmd += ["--sim", SIM]
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        with open(path, "w") as f:
            f.write(r.stdout)
    pts = []
    with open(path) as f:
        for line in f:   # the printed table: rpm, torque N m, power kW, cycles, settled
            m = re.match(r"\s+(\d+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+\d+\s+\w+", line)
            if m:
                pts.append((float(m.group(1)) * RPM, float(m.group(2)), float(m.group(3)) * 1000.0))
    return sorted(pts)


def smoothed(rows, pts):
    """The lead's ruling (18:13): the predicted curve's peaks read as the three-point mean (a real dyno's curve is
    averaged over hundreds of rpm), and the curve's sharpest point (the largest excess of a point over the mean of its
    two neighbours, %) a target of its own, a miss over 5 %. Changes the dyno rows of `rows` in place."""
    # leave out the points on the limiter or below running (torque under a tenth of the curve's best): their cut
    # would make a tooth of the point beside them
    if pts:
        top = max(p[1] for p in pts)
        pts = [p for p in pts if p[1] > 0.1 * top]
    if len(pts) < 3:
        return rows
    sm = [(pts[i][0], (pts[i - 1][1] + pts[i][1] + pts[i + 1][1]) / 3.0, (pts[i - 1][2] + pts[i][2] + pts[i + 1][2]) / 3.0)
          for i in range(1, len(pts) - 1)]
    tq = max(sm, key=lambda p: p[1])
    pw = max(sm, key=lambda p: p[2])
    # the tooth is looked for where a dyno's curve is drawn: from 1500 rpm (the full-load points near idle swing
    # widely: the start of a curve, not its shape)
    band = [p for p in pts if p[0] >= 1500.0 * RPM]
    sharp = max([100.0 * (band[i][1] / (0.5 * (band[i - 1][1] + band[i + 1][1])) - 1.0)
                 for i in range(1, len(band) - 1)] or [0.0])
    vals = {"Peak torque": tq[1], "Speed at peak torque": tq[0], "Peak power": pw[2], "Speed at peak power": pw[0]}
    out = []
    for t, v, st in rows:
        if t["name"] in vals and "dyno" in t and "steps" not in t["dyno"]:
            v = vals[t["name"]]
            st = "pass" if t["range"][0] <= v <= t["range"][1] else "FAIL"
        out.append((t, v, st))
    t = {"name": "The curve's sharpest point", "range": [0, 5], "unit": "%",
         "source": "a real engine's full-load curve has no tooth: the largest excess of any point of the predicted curve "
                   "over the mean of its two neighbours, a miss over 5 % (the lead's ruling, 18:13)"}
    out.append((t, sharp, "pass" if sharp <= 5 else "FAIL"))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default=os.path.join(ROOT, "build-dev", "bin"))
    ap.add_argument("--from", dest="src", default=os.path.join(ROOT, "out", "validation-archetypes"))
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--existing", action="store_true", help="only the archetypes whose tables are in --from already")
    ap.add_argument("--out", default=os.path.join(ROOT, "docs", "manual", "validation.md"))
    ap.add_argument("--sim", default=None, help="the simulator's directory (a worktree pinned at a commit)")
    ap.add_argument("--content", default=None, help="another content/ to run tee-validate on (its builds)")
    ap.add_argument("--commit", default=None, help="the commit the tables were run at (default: HEAD now)")
    a = ap.parse_args()
    global CONTENT, SIM
    CONTENT = a.content
    SIM = a.sim
    os.makedirs(a.src, exist_ok=True)
    slugs = sorted(f[:-5] for f in os.listdir(HERE) if f.endswith(".json"))
    if a.existing:
        slugs = [s for s in slugs if os.path.exists(os.path.join(a.src, s + ".txt"))]
    docs = {s: json.load(open(os.path.join(HERE, s + ".json"))) for s in slugs}
    builds = {s: json.load(open(os.path.join(a.content or os.path.join(ROOT, "content"), docs[s]["build"]))) for s in slugs}
    with ThreadPoolExecutor(a.jobs) as ex:
        texts = dict(zip(slugs, ex.map(lambda s: run(a.bin, s, a.src), slugs)))
    results = {s: parse(texts[s], docs[s]["targets"]) for s in slugs}
    # the idle's standing tone, measured here (tee-validate has no spectral measure)
    content_dir = a.content or os.path.join(ROOT, "content")
    with ThreadPoolExecutor(a.jobs) as ex:
        tones = dict(zip(slugs, ex.map(lambda s: tone(a.bin, s, a.src, os.path.join(content_dir, docs[s]["build"])),
                                       slugs)))
    for s_ in slugs:
        if results[s_] is None:
            continue
        d, hz, cd = tones[s_]
        if cd is not None:
            # a diesel: the clatter's comb (the lead's ruling, 00:33), not the tone against the exhaust
            results[s_].append((COMB, cd, "pass" if COMB["range"][0] <= cd <= COMB["range"][1] else "FAIL"))
        else:
            results[s_].append((dict(TONE, line=hz), d, "pass" if TONE["range"][0] <= d <= TONE["range"][1]
                                else "FAIL"))
    content = a.content or os.path.join(ROOT, "content")
    for s_ in slugs:
        if s_ in TAKEUP_SLUGS and results[s_] is not None:
            # the build's own peak torque: its predicted curve from 600 rpm to its rows' top dyno step (a governed
            # diesel's rows are stepped; its peak sits low, near 1000 rpm)
            tops = [max(t["dyno"]["steps"]) for t in docs[s_]["targets"] if "dyno" in t and "steps" in t["dyno"]]
            plan_ = {"from": 600 * RPM, "to": max(tops)} if tops else None
            if plan_:
                peak = max(p[1] for p in curve(a.bin, s_, a.src, os.path.join(content, docs[s_]["build"]), plan_))
                tp, ts, st_ = takeup(a.bin, s_, a.src, os.path.join(content, docs[s_]["build"]), peak)
                row = dict(TAKEUP, source=TAKEUP["source"] + "; read: to 90 %% of settled %.2f s, settled %.0f N m = "
                           "%.0f %% of the peak %.0f; the climb from 800 rpm (N2's take-up tool) not measured here"
                           % (ts, st_, 100.0 * st_ / peak, peak))
                results[s_].append((row, tp, "pass" if TAKEUP["range"][0] <= tp <= TAKEUP["range"][1] else "FAIL"))
        if s_ in CROSS_SLUGS and results[s_] is not None:
            x = crossing(a.bin, s_, a.src, os.path.join(content, docs[s_]["build"]))
            if x is not None:
                results[s_].append((CROSS, x, "pass" if CROSS["range"][0] <= x <= CROSS["range"][1] else "FAIL"))
    for s_ in slugs:
        plan = next((t["dyno"] for t in docs[s_]["targets"] if "dyno" in t and "steps" not in t["dyno"]), None)
        if plan and results[s_] is not None:
            pts = curve(a.bin, s_, a.src, os.path.join(content, docs[s_]["build"]), plan)
            results[s_] = smoothed(results[s_], pts)

    commit = a.commit or subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True,
                                        cwd=ROOT).stdout.strip()
    out = ["# How right is it?", "", "*Run at commit `%s`.*" % commit, "",
           "*Generated by `content/validation/archetypes/report.py` from `tee-validate --simulator` on each archetype "
           "(`content/validation/archetypes/`). Do not edit by hand: run it again.*", "",
           "Each archetype build is held to what an engine and car of its kind measure as: published figures, "
           "textbook ranges and rules of thumb, each target with its source in its file. Only the invariants (it "
           "starts, stays within full scale, holds its limiter, pulls away, stops at the key) must pass; every other "
           "miss is reported, not failed. **The misses are the simulator's to-do list.**", "",
           "Peak power and torque: naturally aspirated archetypes on the predicted dyno curve (steady full-throttle points, "
           "warm, `tee-dyno --predict` at tee-validate's 48 points), read as its THREE-POINT MEAN (a real dyno's curve is "
           "averaged over hundreds of rpm: one sharp point must not decide a target), with the curve's sharpest point a "
           "target of its own (a miss over 5 %); boosted ones on the full engine dyno, nine steps held 5 s and measured 1 s "
           "(the predicted curve reads a boosted engine some 10 % high: its slow states do not settle). The rest "
           "on the scenarios of "
           "`content/validation/scenarios/` (`p-*`), in the build's own air (15 C, 101.3 kPa).", ""]

    # the invariants that fail, by name, at the top (the lead's ask: a regression must be visible here, not in row 400)
    failing = {}
    for s_ in slugs:
        for t, v, st in results[s_] or []:
            if t["name"].startswith("Invariant") and st != "pass":
                failing.setdefault(t["name"].replace("Invariant: ", ""), []).append(builds[s_]["name"])
    if failing:
        out += ["**Invariants failing:** " + "; ".join("%s: %s" % (k, ", ".join(v)) for k, v in failing.items()) + ".", ""]
    else:
        out += ["**Invariants failing:** none: every archetype starts warm and runs, stays within full scale, holds its "
                "limiter, pulls away and stops at the key.", ""]

    # the summary
    out += ["## The archetypes at a glance", "",
            "| Archetype | Peak power (target) | Peak torque (target) | Warm idle (target) | Quarter mile | Top speed | Invariants |",
            "|---|---|---|---|---|---|---|"]
    def cell(rows, name):
        for t, v, st in rows:
            if t["name"] == name:
                mark = "" if st == "pass" else (" **miss**" if st == "FAIL" else " n/a")
                return "%s (%s)%s" % (show(t, v), rng(t), mark)
        return "-"
    for s in slugs:
        rows = results[s]
        if rows is None:
            out.append("| %s | did not run | | | | | |" % builds[s]["name"])
            continue
        inv = [st for t, v, st in rows if t["name"].startswith("Invariant")]
        qt = [(t, v, st) for t, v, st in rows if t["name"].startswith("Quarter mile")]
        q = "-" if not qt else "%s at %s%s" % (show(qt[0][0], qt[0][1]), show(qt[1][0], qt[1][1]),
                                                "" if all(x[2] == "pass" for x in qt) else " **miss**")
        out.append("| %s | %s | %s | %s | %s | %s | %d of %d |" % (
            builds[s]["name"], cell(rows, "Peak power"), cell(rows, "Peak torque"), cell(rows, "Warm idle"), q,
            cell(rows, "Top speed"), inv.count("pass"), len(inv)))
    out.append("")

    # the misses, by target
    by = {}
    for s in slugs:
        for t, v, st in results[s] or []:
            if st != "pass":
                by.setdefault(t["name"], []).append((s, t, v, st))
    total = sum(len(r or []) for r in results.values())
    missed = sum(len(v) for v in by.values())
    out += ["## The misses, target by target", "",
            "%d targets over %d archetypes; %d missed or not measurable. Each line: the archetype, what was measured, "
            "the range it should be in." % (total, len(slugs), missed), ""]
    # what the table is measured against (the lead, 20:23): every target by the kind of its source, and its misses
    kinds = {"cited": [0, 0], "recalled": [0, 0], "judgement": [0, 0]}
    for s in slugs:
        for t, v, st in results[s] or []:
            k = kinds[basis(t)]
            k[0] += 1
            k[1] += st != "pass"
    line = ("**Missed: %d of %d** (%d of %d cited, %d of %d recalled, %d of %d judgement). *Cited*: a page, a "
            "figure, a standard or a maker's datum looked up and named in its source; *recalled*: a maker's figure or "
            "a textbook range written from memory, believed right; *judgement*: a band set because it seemed "
            "reasonable, which catches a wild value but is no reason to move the model toward it." %
            (missed, total, kinds["cited"][1], kinds["cited"][0], kinds["recalled"][1], kinds["recalled"][0],
             kinds["judgement"][1], kinds["judgement"][0]))
    out[out.index("## The archetypes at a glance")] = line + "\n\n## The archetypes at a glance"
    out += [line, ""]
    # the worst five, by how far outside its range, with each one's kind
    worst = []
    for s in slugs:
        for t, v, st in results[s] or []:
            if st == "FAIL" and v is not None and "range" in t:
                lo, hi = sorted(t["range"])
                ref = lo if v < lo else hi
                if ref:
                    worst.append((abs(v - ref) / abs(ref), s, t, v))
    worst.sort(key=lambda w: -w[0])
    out += ["**The worst five** (by how far outside the range, as a share of its nearer edge):", ""]
    out += ["%d. %s, %s: %s against %s (%.0f %% out; %s)" % (i + 1, builds[s]["name"], t["name"], show(t, v), rng(t),
                                                            100 * d, basis(t)) for i, (d, s, t, v) in enumerate(worst[:5])]
    out.append("")
    order = sorted(by, key=lambda n: (not n.startswith("Invariant"), -len(by[n])))
    for name in order:
        out += ["### %s (%d; %s)" % (name, len(by[name]), basis(by[name][0][1])), ""]
        src = by[name][0][1]["source"]
        out += ["*Source: %s.*" % src.rstrip("."), ""] if name not in ("Peak power", "Peak torque", "Speed at peak power",
                                                                      "Speed at peak torque") else \
               ["*Source: each kind's published figures, named in its build's notes and its validation file.*", ""]
        out += ["| Archetype | Measured | Should be |", "|---|---|---|"]
        for s, t, v, st in by[name]:
            out.append("| %s | %s%s | %s |" % (builds[s]["name"], show(t, v), " (n/a)" if st == "n/a" else "", rng(t)))
        out.append("")
    with open(a.out, "w") as f:
        f.write("\n".join(out))
    print("%d archetypes, %d targets, %d missed" % (len(slugs), total, missed))


if __name__ == "__main__":
    main()
