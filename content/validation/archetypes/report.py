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
        rows.append((t, val, "FAIL" if m.group(2) == "MISS" else m.group(2)))
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
    content = a.content or os.path.join(ROOT, "content")
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
