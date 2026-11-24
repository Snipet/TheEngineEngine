#!/usr/bin/env python3
"""The loss ledger (card N): where an archetype's torque goes, line by line, beside the range a textbook gives.

    python3 scripts/ledger.py economy-four-15 straight-six-32-high-rev flat-plane-v8-45
    python3 scripts/ledger.py --na                    every naturally aspirated petrol archetype
    python3 scripts/ledger.py --boost                 every boosted one; --all for the thirty
    python3 scripts/ledger.py v8x --set engine.wallHeat=1 --set fuel.model=0 --out out/ledger-try.md

Each build is run on the engine dyno (tee-dyno --steps: MODE_DYNO_ENGINE, full throttle, warm) at five speeds from
1500 rpm to just under its limiter, and the telemetry the modules report is averaged over each step. From it: the
volumetric efficiency against the manifold's density and the outside air's (the air trapped at intake closing); gross
IMEP, pumping, friction and brake MEP; the brake efficiency; lambda; the spark and the half-burn; the charge at intake
closing; the manifold; the exhaust port; and, boosted, the machine's lines. A line whose telemetry does not exist yet
reads n/a with the field that is wanted.

Output: markdown (stdout, or --out FILE; the card's file is docs/measurements/calibration/<na|boost>/ledger.md,
generated with --write) and the raw numbers as JSON (--json FILE) for tests and for another script.

Ranges: a line's range is for the class of engine (naturally aspirated petrol, boosted petrol, diesel), with its
source. The ranges are wide on purpose: a line outside one is a lead, not a verdict. Card N1 owns this file; N2 adds
its boosted lines by telling N1.
"""
import argparse
import csv
import glob
import json
import math
import os
import subprocess
import sys
import tempfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ARCH = os.path.join(ROOT, "content", "builds", "archetypes")
RPM = math.pi / 30.0
R_AIR = 287.05

# Lower heating values, J/kg (Heywood, Internal Combustion Engine Fundamentals, appendix D; Bosch Automotive Handbook).
LHV = {"Gasoline": 43.4e6, "Diesel": 42.8e6}
LHV_GRADE = {"E85": 29.2e6, "Methanol": 20.0e6, "Propane (untested)": 46.4e6, "Diesel": 42.8e6}

HEYWOOD = "Heywood, Internal Combustion Engine Fundamentals"
BLAIR = "Blair, Design and Simulation of Four-Stroke Engines"
BOSCH = "Bosch Automotive Handbook"
TAYLOR = "Taylor, The Internal-Combustion Engine in Theory and Practice"

# The telemetry recorded at each step (tel. is implied). Missing fields are dropped with a note.
FIELDS = [
    "engine.speed", "engine.imep", "engine.pmep", "engine.fmep", "engine.bmep", "engine.veManifold", "engine.veAir",
    "engine.manifold", "engine.peakPressure", "engine.pistonSpeed", "engine.gasTorque", "intake.manifold",
    "ecu.lambda", "ecu.lambdaTarget", "ecu.advance", "ecu.advanceAsked", "ecu.halfBurn", "ecu.fuelFlow",
    "ecu.knockRetard", "ecu.peakPressure", "ecu.peakAngle", "exhaust.portTemperature",
    "boost.pressure", "boost.ratio", "boost.flow", "boost.efficiency", "boost.shaftSpeed", "boost.compressorOut",
    "boost.intercoolerOut", "boost.throttleIn", "boost.turbineIn", "boost.turbineInTemp", "boost.gate",
    "boost.superchargerPower",
    # wanted, not yet in the telemetry (asked of their owners): read when they appear
    "engine.chargeTemperature", "engine.trappedMass", "exhaust.portPressure",
]


def fm(x, digits=2):
    return "-" if x is None or (isinstance(x, float) and math.isnan(x)) else f"{x:.{digits}f}"


def load(slug):
    path = slug if slug.endswith(".json") else os.path.join(ARCH, slug + ".json")
    with open(path) as f:
        return path, json.load(f)


def put(spec, key, text):
    """--set section.field=value into the build's spec (a number when it reads as one, else a choice's name)."""
    try:
        value = json.loads(text)
    except ValueError:
        value = text
    parts = key.split(".")
    if parts[0] == "spec":
        parts = parts[1:]
    node = spec
    for p in parts[:-1]:
        node = node.setdefault(p, {})
    node[parts[-1]] = value


def kind_of(spec):
    boost = spec.get("boost", {}).get("kind", "Natural")
    fuel = spec.get("fuel", {}).get("kind", "Gasoline")
    if fuel == "Diesel" or spec.get("fuel", {}).get("grade") == "Diesel":
        return "diesel"
    return "na" if boost in ("Natural", 0, None) else "boost"


def speeds_of(spec, n):
    red = spec["engine"].get("redlineSpeed", 680.0)
    lim = spec.get("ecu", {}).get("limiter", {}).get("speed", red) or red
    top = 0.96 * min(red, lim)
    lo = 1500 * RPM
    if top <= lo:
        lo = 0.4 * top
    return [lo + (top - lo) * i / (n - 1) for i in range(n)]


def find_tool(name, bin_dir):
    for d in ([bin_dir] if bin_dir else []) + sorted(glob.glob(os.path.join(ROOT, "build-*", "bin"))):
        p = os.path.join(d, name)
        if os.path.exists(p):
            return p
    sys.exit(f"ledger: no {name} (build it: cmake --build --preset dev --target {name}); or --bin DIR")


def known_fields(tee_schema, sim=""):
    """The tel fields the simulator has (tee-schema --json): {name without tel.: the name to ask the dyno for}, an
    array's first element (a turbo's [0])."""
    try:
        text = subprocess.run([tee_schema, "--json"] + (["--sim", sim] if sim else []), capture_output=True, text=True, cwd=ROOT).stdout
        doc = json.loads(text)
    except (OSError, ValueError):
        return None
    out = {}
    for f in doc.get("fields", []):
        path = f.get("path", "")
        if path.startswith("tel."):
            out[path[4:]] = path[4:] + ("[0]" if f.get("kind") == "array" else "")
    return out or None


def run_dyno(dyno, spec_doc, speeds, fields, settle, measure, sim=""):
    with tempfile.TemporaryDirectory() as tmp:
        build = os.path.join(tmp, "build.json")
        with open(build, "w") as f:
            json.dump(spec_doc, f)
        out = os.path.join(tmp, "dyno.csv")
        cmd = [dyno, "--build", build, "--steps", ",".join(f"{w:.3f}" for w in speeds), "--settle", str(settle),
               "--measure", str(measure), "--fields", ",".join(fields), "--csv", out, "--quiet"] + (["--sim", sim] if sim else [])
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        if r.returncode != 0:
            raise RuntimeError(r.stderr.strip() or r.stdout.strip())
        with open(out) as f:
            return list(csv.DictReader(f))


class Line:
    def __init__(self, key, label, unit, digits, ranges, source, note=""):
        self.key, self.label, self.unit, self.digits = key, label, unit, digits
        self.ranges, self.source, self.note = ranges, source, note

    def range_for(self, kind, rpm):
        r = self.ranges.get(kind, self.ranges.get("*"))
        return r(rpm) if callable(r) else r


def chen_flynn_heywood(rpm):
    # Heywood eq. 13.15 (after Barnes-Moss): total friction MEP of four-stroke petrol engines of 0.85 to 2.0 L at
    # wide-open throttle, bar = 0.97 + 0.15 (N/1000) + 0.05 (N/1000)^2; held to within 25 %.
    k = rpm / 1000.0
    f = 0.97 + 0.15 * k + 0.05 * k * k
    return (0.75 * f, 1.25 * f)


LINES = [
    Line("bmep", "BMEP", "bar", 2, {"na": (10.0, 14.5), "boost": (14.0, 26.0), "diesel": (12.0, 24.0)},
         f"{HEYWOOD}, §2.7 and table 1-1: production engines at peak torque, NA petrol 10 to 13.5, high-output "
         "NA to 14.5; boosted petrol 15 to 25; the class's own figure is in the archetype's targets"),
    Line("imep", "Gross IMEP", "bar", 2, {"na": (11.5, 16.5), "boost": (16.0, 29.0), "diesel": (13.5, 27.0)},
         f"{HEYWOOD}, §2.7, §13.2: gross IMEP = BMEP + friction + pumping at full load"),
    Line("pmep", "Pumping MEP", "bar", 2, {"na": (0.05, 0.6), "boost": (-0.8, 0.6), "diesel": (-0.5, 0.6)},
         f"{HEYWOOD}, §13.4, fig. 13-11: at wide-open throttle the gas exchange takes 0.1 to 0.5 bar, rising with "
         "speed; boosted, negative while the boost exceeds the back pressure"),
    Line("fmep", "Friction MEP", "bar", 2, {"*": (0.0, 1e9)},
         "the engine's own friction (core/friction.fm, Chen and Flynn's form); judged with the pumping on the next line"),
    Line("fpmep", "Friction + pumping MEP", "bar", 2, {"*": chen_flynn_heywood},
         f"{HEYWOOD}, eq. 13.15 (after Barnes-Moss): the motored total, friction with the pumping, of petrol fours of 0.85 "
         "to 2.0 L at wide-open throttle, 0.97 + 0.15 (N/1000) + 0.05 (N/1000)^2 bar, held to within 25 %"),
    Line("veAir", "Vol. eff. (outside air)", "", 3, {"na": (0.80, 1.05), "boost": (1.2, 3.0), "diesel": (0.85, 3.0)},
         f"{HEYWOOD}, §6.2, fig. 6-9 and 6-10: 0.80 to 0.90 for production two- and four-valve engines at peak "
         f"torque, to 1.0 to 1.1 with tuned runners ({BLAIR}, ch. 6)"),
    Line("veMan", "Vol. eff. (manifold)", "", 3, {"*": (0.80, 1.12)},
         f"{HEYWOOD}, §6.2: against the manifold's density; ram and a tuned exhaust lift it past 1 ({BLAIR}, ch. 6); "
         "computed as veAir x (p_air / p_manifold) x (T_manifold / T_air), T_manifold the throttle's (boosted) or "
         "the air's"),
    Line("veTel", "Vol. eff. (manifold), tel", "", 3, {"*": (0.80, 1.12)},
         "tel.engine.veManifold as the engine reports it (card D's; read beside the computed line)"),
    Line("effBrake", "Brake efficiency", "", 3, {"na": (0.27, 0.37), "boost": (0.26, 0.37), "diesel": (0.33, 0.45)},
         f"{HEYWOOD}, §2.10, fig. 15-? and {BOSCH}: best BSFC 230 to 270 g/kWh petrol at part load; at full load, "
         "rich, 0.28 to 0.35; diesels 0.35 to 0.44"),
    Line("lambda", "Lambda", "", 3, {"na": (0.78, 1.0), "boost": (0.72, 0.95), "diesel": (1.15, 2.2)},
         f"{HEYWOOD}, §7.1 and {BOSCH}: power enrichment 0.80 to 0.90 at full load; boosted richer for the "
         "turbine's inlet; a diesel's smoke limit 1.2 to 1.5 at peak torque"),
    Line("advance", "Spark", "deg BTDC", 1, {"na": (10.0, 40.0), "boost": (0.0, 30.0), "diesel": (-10.0, 25.0)},
         f"{HEYWOOD}, §9.3: MBT 15 to 35 deg at full load; a boosted engine knock-limited lower"),
    Line("halfBurn", "Half-burn", "deg ATDC", 1, {"*": (4.0, 12.0)},
         f"{HEYWOOD}, §9.3: at MBT half the charge is burned at 8 to 10 deg after the top (card H measured the "
         "model's best at 3 to 5, flat within 0.3 % from 0 to 8)"),
    Line("peakP", "Peak cylinder pressure", "bar", 1, {"na": (45.0, 95.0), "boost": (70.0, 140.0), "diesel": (80.0, 220.0)},
         f"{HEYWOOD}, §9.2; {TAYLOR}, vol. 1: NA petrol 50 to 90 bar at full load, boosted 90 to 130"),
    Line("tCharge", "Charge at intake closing", "K", 0, {"na": (320.0, 400.0), "boost": (320.0, 420.0), "diesel": (320.0, 420.0)},
         f"{HEYWOOD}, §6.4 and §13.? : fresh charge heated by the walls and mixed with the residual, 330 to 400 K at "
         "full load", "wants tel.engine.chargeTemperature (card D)"),
    Line("manifold", "Manifold", "kPa", 1, {"na": (93.0, 101.3), "boost": (130.0, 300.0), "diesel": (100.0, 400.0)},
         f"{HEYWOOD}, §6.2: at wide-open throttle the manifold sits a few kPa below the air"),
    Line("portP", "Exhaust port, mean", "kPa", 1, {"na": (102.0, 135.0), "boost": (120.0, 350.0), "diesel": (110.0, 400.0)},
         f"{HEYWOOD}, §6.5; {BOSCH}: a road exhaust's back pressure 10 to 30 kPa over the air at rated power",
         "wants tel.exhaust.portPressure (card G)"),
    Line("tPort", "Port gas", "K", 0, {"na": (1000.0, 1300.0), "boost": (1000.0, 1350.0), "diesel": (750.0, 1050.0)},
         f"{HEYWOOD}, §14.? and fig. 6-?: a petrol engine's gas leaving the port at full load 1100 to 1300 K "
         "(the header thermocouple's 900 to 1100 K is further down); diesel 750 to 1000 K"),
    # boosted (card N2 adds and tunes these)
    Line("boostOver", "Boost over target", "", 2, {"boost": (0.9, 1.1)},
         "the boost (tel.boost.pressure, over the air) over the build's spec.boost.target: a runaway, or a turbo out of "
         "flow, shows as this line out of range (card N2)"),
    Line("pr", "Pressure ratio", "", 2, {"boost": (1.3, 2.8)}, f"{HEYWOOD}, §6.8; {BOSCH}"),
    Line("flow", "Compressor flow", "kg/s", 3, {"boost": (0.0, 2.0)}, "the engine's air, for the map"),
    Line("compEff", "Compressor efficiency", "", 3, {"boost": (0.60, 0.80)},
         f"{HEYWOOD}, §6.8 and Watson and Janota, Turbocharging the Internal Combustion Engine: 0.65 to 0.78 on the map's island"),
    Line("shaft", "Shaft speed", "krpm", 0, {"boost": (40.0, 250.0)}, "Watson and Janota: small to large automotive turbochargers"),
    Line("tIc", "Intercooler out", "K", 0, {"boost": (295.0, 340.0)}, f"{BOSCH}: an air-to-air cooler 60 to 80 % effective"),
    Line("p3", "Turbine inlet", "kPa", 1, {"boost": (120.0, 400.0)},
         "Watson and Janota: a small turbo's turbine inlet runs 1.0 to 1.5 times the boost at full load"),
    Line("p3p2", "Turbine inlet / boost", "", 2, {"boost": (0.95, 1.6)}, "Watson and Janota (as above)"),
    Line("gate", "Gate opening", "", 2, {"boost": (0.0, 1.0)}, "the controller's"),
    Line("scPower", "Supercharger drive", "kW", 1, {"boost": (0.0, 1e9)},
         f"{BOSCH}: a positive-displacement blower takes 10 to 20 % of the engine's power at full boost"),
]


def ledger_points(rows, spec, kind):
    env = spec.get("env", {})
    p_air = float(env.get("pressure", 101325.0)) if isinstance(env.get("pressure"), (int, float)) else 101325.0
    t_air = float(env.get("temperature", 288.15)) if isinstance(env.get("temperature"), (int, float)) else 288.15
    vd = float(spec["engine"]["displacement"])
    fuel = spec.get("fuel", {})
    lhv = LHV_GRADE.get(fuel.get("grade"), LHV.get(fuel.get("kind", "Gasoline"), 43.4e6))
    pts = []
    for r in rows:
        def g(k, r=r):
            v = r.get(k, r.get(k + "[0]"))
            return float(v) if v not in (None, "", "nan") else None
        rpm = g("rpm")
        torque = g("torque_N_m")
        power = (g("power_kW") or 0.0) * 1e3
        man = g("intake.manifold") or g("engine.manifold")
        tman = None
        if kind != "na":
            tman = g("boost.throttleIn") or g("boost.intercoolerOut")
        tman = tman or t_air
        ve_air = g("engine.veAir")
        p = {
            "rpm": rpm, "torque": torque, "power": power / 1e3,
            "bmep": 4 * math.pi * torque / vd / 1e5 if torque is not None else None,
            "bmepTel": (g("engine.bmep") or 0) / 1e5,
            "imep": g("engine.imep") / 1e5 if g("engine.imep") is not None else None,
            "pmep": g("engine.pmep") / 1e5 if g("engine.pmep") is not None else None,
            "fmep": g("engine.fmep") / 1e5 if g("engine.fmep") is not None else None,
            "fpmep": (g("engine.fmep") + g("engine.pmep")) / 1e5 if g("engine.fmep") is not None and g("engine.pmep") is not None else None,
            "veAir": ve_air,
            "veMan": ve_air * (p_air / man) * (tman / t_air) if ve_air is not None and man else None,
            "veTel": g("engine.veManifold"),
            "effBrake": power / (g("ecu.fuelFlow") * lhv) if g("ecu.fuelFlow") else None,
            "lambda": g("ecu.lambda"),
            "advance": g("ecu.advance"),
            "halfBurn": g("ecu.halfBurn"),
            "peakP": (g("engine.peakPressure") or 0) / 1e5 or None,
            "tCharge": g("engine.chargeTemperature"),
            "manifold": man / 1e3 if man else None,
            "portP": g("exhaust.portPressure") / 1e3 if g("exhaust.portPressure") else None,
            "tPort": g("exhaust.portTemperature"),
        }
        if kind != "na":
            b = g("boost.pressure")
            p3 = g("boost.turbineIn")
            tgt = float(spec.get("boost", {}).get("target", 0) or 0)
            p.update({
                "boostOver": b / tgt if b is not None and tgt > 0 else None,
                "pr": g("boost.ratio"), "flow": g("boost.flow"), "compEff": g("boost.efficiency"),
                "shaft": g("boost.shaftSpeed") * 60 / (2 * math.pi) / 1e3 if g("boost.shaftSpeed") else None,
                "tIc": g("boost.intercoolerOut"),
                "p3": p3 / 1e3 if p3 else None,
                "p3p2": p3 / b if p3 and b else None,
                "gate": g("boost.gate"),
                "scPower": g("boost.superchargerPower") / 1e3 if g("boost.superchargerPower") else None,
            })
        p["fuelFlow"] = g("ecu.fuelFlow")
        p["knockRetard"] = g("ecu.knockRetard")
        p["lambdaTarget"] = g("ecu.lambdaTarget")
        p["advanceAsked"] = g("ecu.advanceAsked")
        pts.append(p)
    return pts


def markdown(name, slug, kind, pts, notes):
    out = [f"### {name} (`{slug}`, {dict(na='naturally aspirated petrol', boost='boosted petrol', diesel='diesel')[kind]})", ""]
    out += notes and [" ".join(notes), ""] or []
    head = "| Line | " + " | ".join(f"{p['rpm']:.0f} rpm" for p in pts) + " | Range | |"
    out += [head, "|---" * (len(pts) + 3) + "|"]
    out.append("| Torque, N m | " + " | ".join(fm(p["torque"], 1) for p in pts) + " | | |")
    out.append("| Power, kW | " + " | ".join(fm(p["power"], 1) for p in pts) + " | | |")
    marks = []
    for ln in LINES:
        if ln.range_for(kind, 3000) is None:
            continue
        cells, bad = [], 0
        for p in pts:
            v = p.get(ln.key)
            lo, hi = ln.range_for(kind, p["rpm"])
            if v is None:
                cells.append("n/a")
            elif v < lo or v > hi:
                cells.append(f"**{fm(v, ln.digits)}**")
                bad += 1
            else:
                cells.append(fm(v, ln.digits))
        rng = ln.range_for(kind, 3000)
        rtext = "Heywood 13.15 ±25 %" if callable(ln.ranges.get("*")) else f"{fm(rng[0], ln.digits)} to {fm(rng[1], ln.digits)}"
        if not callable(ln.ranges.get("*")) and rng[1] >= 1e8:
            rtext = "(reported)"
        unit = f", {ln.unit}" if ln.unit else ""
        flag = "out" if bad else ""
        if all(c == "n/a" for c in cells) and ln.note:
            flag = ln.note
        out.append(f"| {ln.label}{unit} | " + " | ".join(cells) + f" | {rtext} | {flag} |")
    return "\n".join(out) + "\n"


def sources():
    out = ["### The ranges' sources", ""]
    for ln in LINES:
        out.append(f"- **{ln.label}**: {ln.source}.")
    return "\n".join(out) + "\n"


def commit():
    try:
        return subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True, cwd=ROOT).stdout.strip()
    except OSError:
        return "?"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("builds", nargs="*", help="archetype slugs (content/builds/archetypes/<slug>.json) or build files")
    ap.add_argument("--na", action="store_true", help="every naturally aspirated petrol archetype")
    ap.add_argument("--boost", action="store_true", help="every boosted petrol archetype")
    ap.add_argument("--diesel", action="store_true", help="every diesel archetype")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--set", action="append", default=[], metavar="KEY=VALUE",
                    help="change a build's field for the experiment (engine.wallHeat=1, valve.model=0, ...)")
    ap.add_argument("--points", type=int, default=5)
    ap.add_argument("--rpm", default="", help="the speeds, rpm, comma-separated (else --points from 1500 to the limiter)")
    ap.add_argument("--settle", type=float, default=-1.0,
                    help="seconds each step is held before it is measured (default 1.2; a boosted build 4.5: its knock "
                         "retard and its intercooler take seconds, card N2)")
    ap.add_argument("--measure", type=float, default=1.0, help="seconds averaged (the engine is chaotic: means over cycles)")
    ap.add_argument("--bin", default="", help="the directory of tee-dyno and tee-schema (else build-*/bin)")
    ap.add_argument("--sim", default="", help="the simulator's directory (else this checkout's sim/)")
    ap.add_argument("--out", default="", help="markdown to this file (else stdout)")
    ap.add_argument("--json", default="", help="the numbers as JSON to this file")
    ap.add_argument("--write", action="store_true",
                    help="write docs/measurements/calibration/<na|boost>/ledger.md (and ledger.json) for the class")
    ap.add_argument("--jobs", type=int, default=3)
    a = ap.parse_args()

    slugs = list(a.builds)
    every = sorted(os.path.basename(p)[:-5] for p in glob.glob(os.path.join(ARCH, "*.json")))
    for s in every:
        k = kind_of(load(s)[1]["spec"])
        if a.all or (a.na and k == "na") or (a.boost and k == "boost") or (a.diesel and k == "diesel"):
            if s not in slugs:
                slugs.append(s)
    if not slugs:
        ap.error("name a build, or --na, --boost, --diesel, --all")

    dyno = find_tool("tee-dyno", a.bin)
    have = known_fields(find_tool("tee-schema", a.bin), a.sim)
    if have is None:
        sys.exit("ledger: tee-schema could not read the simulator (does it compile?)")
    fields = [have[f] if have else f for f in FIELDS if have is None or f in have]
    missing = [f for f in FIELDS if have is not None and f not in have]

    def one(slug):
        path, doc = load(slug)
        for kv in a.set:
            k, _, v = kv.partition("=")
            put(doc["spec"], k, v)
        spec = doc["spec"]
        kind = kind_of(spec)
        speeds = [float(x) * RPM for x in a.rpm.split(",")] if a.rpm else speeds_of(spec, a.points)
        settle = a.settle if a.settle >= 0 else (1.2 if kind == "na" else 4.5)
        try:
            rows = run_dyno(dyno, doc, speeds, fields, settle, a.measure, a.sim)
        except RuntimeError as e:
            return slug, doc.get("name", slug), kind, None, str(e)
        return slug, doc.get("name", slug), kind, ledger_points(rows, spec, kind), ""

    from concurrent.futures import ThreadPoolExecutor
    with ThreadPoolExecutor(max_workers=max(1, a.jobs)) as pool:
        results = list(pool.map(one, slugs))

    md = ["# The loss ledger", "",
          "*Generated by `scripts/ledger.py`" + (f" with `{' '.join('--set ' + s for s in a.set)}`" if a.set else "") +
          ": do not edit by hand. The engine dyno (MODE_DYNO_ENGINE, full throttle, warm), each speed held "
          f"{a.settle if a.settle >= 0 else '1.2 (boosted 4.5)'} s and averaged over {a.measure} s, on commit "
          f"{commit()}{' with the simulator of ' + a.sim if a.sim else ''}. **Bold**: outside the class's range. "
          "Each range's source is at the end. Under fuel.model 1 the engine's one decision (comb.walls, D) runs "
          "Woschni's walls with the burn's 0.97 share and Chen-Flynn friction, whatever engine.wallHeat says.*", ""]
    if missing:
        md += ["Telemetry wanted and not yet there (its line reads n/a): " + ", ".join(f"`tel.{m}`" for m in missing) + ".", ""]
    data = {}
    for slug, name, kind, pts, err in results:
        if pts is None:
            md += [f"### {name} (`{slug}`)", "", f"The dyno failed: {err}", ""]
            continue
        notes = []
        peak = max(pts, key=lambda p: p["torque"] or -1e9)
        notes.append(f"Peak of these points: {peak['torque']:.0f} N m ({peak['bmep']:.2f} bar BMEP) at {peak['rpm']:.0f} rpm; "
                     f"{max(p['power'] for p in pts):.0f} kW.")
        md.append(markdown(name, slug, kind, pts, notes))
        data[slug] = {"name": name, "kind": kind, "set": a.set, "points": pts}
    md.append(sources())
    text = "\n".join(md)

    if a.write:
        kinds = {d["kind"] for d in data.values()}
        sub = "na" if kinds <= {"na"} else "boost"
        base = os.path.join(ROOT, "docs", "measurements", "calibration", sub)
        os.makedirs(base, exist_ok=True)
        a.out = a.out or os.path.join(base, "ledger.md")
        a.json = a.json or os.path.join(base, "ledger.json")
    if a.out:
        os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
        with open(a.out, "w") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    if a.json:
        with open(a.json, "w") as f:
            json.dump(data, f, indent=1)
    return 0 if all(r[3] is not None for r in results) else 1


if __name__ == "__main__":
    sys.exit(main())
