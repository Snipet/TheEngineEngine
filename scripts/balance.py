#!/usr/bin/env python3
"""scripts/balance.py: card N3's air and energy balance of the naturally aspirated archetypes (card N, package N3).

For each archetype asked, at the speeds of its targets' peak torque and peak power (the middle of each target's
range: content/validation/archetypes/<slug>.json), it runs the balance probe (tests/sim/calibration/balance, hidden tag
[.balance]: the engine dyno at full throttle, warm, the exhaust's and the intake's heat settled as tee-dyno --steps
settles them, the books read from the cylinders' own state each sample) and writes

    docs/measurements/calibration/balance/balance.md     (generated: never edited by hand)

with the air's books in g per cycle per litre of displacement and the energy's as shares of the fuel's lower heating
value, each beside the range a textbook gives for a spark-ignition engine at full load, with its source.

    scripts/balance.py [--bin build-dev/bin] [--only slug,slug] [--out FILE] [--set "k=v;k=v"] [--sim DIR]
"""
import argparse
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SEVEN = ["air-cooled-flat-six-36", "vr6-28", "twin-cam-roadster-18", "small-block-57-carb", "pushrod-v6-38",
         "big-bore-70-pushrod-v8", "four-cam-v8-50"]
HEYWOOD = "Heywood, Internal Combustion Engine Fundamentals (1988)"

# The energy's lines: key, label, range (shares of the fuel's LHV) or None, source.
ENERGY = [
    ("mix_heat", "Heat the mixture can release at its lambda", (0.86, 0.93),
     f"{HEYWOOD}, §4.9.4 and fig. 4-23 (combustion efficiency): at phi 1.1 the oxygen limits the release; the "
     "CO and H2 in the products carry 7 to 12 % of the fuel's heat (eq. 4.69's chemistry, worked in causes.md)"),
    ("released", "Released into the gas", (0.84, 0.92),
     f"{HEYWOOD}, fig. 4-23 with §11's unburned HC (2 to 3 % at full load)"),
    ("wall_compression", "Walls, compression", None, "reported"),
    ("wall_burn", "Walls, the burn", None, "reported"),
    ("wall_expansion", "Walls, expansion", None, "reported"),
    ("wall_exchange", "Walls, gas exchange", None, "reported"),
    ("walls", "Walls, in the cylinder, all", (0.15, 0.25),
     f"{HEYWOOD}, §12.1 and table 12.1: 17 to 26 % to the coolant at full power, less the exhaust port's 3 to 5 % "
     "and friction's share"),
    ("work_gross", "Gross indicated work", (0.33, 0.38),
     f"{HEYWOOD}, §5.7 fig. 5-9 and §15 (the real engine is some 0.8 of the fuel-air cycle: 0.42 to 0.45 at r 10 to "
     "11, phi 1.1); fig. 15-? measured full-load indicated efficiencies"),
    ("work_pump", "Pumping", (0.005, 0.03), f"{HEYWOOD}, §13.4: 0.1 to 0.6 bar at wide-open throttle"),
    ("friction", "Friction (gross - pumping - brake)", (0.03, 0.07), f"{HEYWOOD}, §13.6 (Chen and Flynn)"),
    ("brake", "Brake", (0.25, 0.31), f"{HEYWOOD}, table 12.1: 25 to 28 % at maximum power; to 31 at peak torque"),
    ("exhaust_sensible", "Exhaust, sensible enthalpy above the inlet's", (0.34, 0.45),
     f"{HEYWOOD}, table 12.1: 34 to 45 % at maximum power"),
    ("first_law", "First law's residual (released - walls - work - net enthalpy - dU)", (-0.01, 0.01),
     "the probe's own check"),
]
AIR = [("air_throttle", "Through the throttle"), ("air_valves", "In through the intake valves, net"),
       ("trapped_fresh", "Trapped at intake closing, fresh"), ("trapped_burned", "Trapped, burned (residual)"),
       ("trapped_total", "Trapped, total"), ("gas_exhaust", "Out through the exhaust valves, net"),
       ("fuel_air", "The air the ECU metered fuel for")]


def find_tool(name, bin_dir):
    for d in [bin_dir, os.path.join(ROOT, "build-dev", "bin")]:
        if d and os.path.exists(os.path.join(d, name)):
            return os.path.join(d, name)
    sys.exit(f"{name} not found: build it (cmake --build --preset dev --target {name})")


def peaks(slug):
    """The middle of the targets' speeds at peak torque and at peak power (rpm; the targets hold rad/s)."""
    doc = json.load(open(os.path.join(ROOT, "content", "validation", "archetypes", slug + ".json")))
    out = {}
    for t in doc["targets"]:
        if t["name"] == "Speed at peak torque":
            out["torque"] = round(sum(t["range"]) / 2 * 30 / 3.141592653589793)
        elif t["name"] == "Speed at peak power":
            out["power"] = round(sum(t["range"]) / 2 * 30 / 3.141592653589793)
    return out


def run(probe, slug, rpms, setting, sim):
    env = dict(os.environ, BAL_BUILD=slug, BAL_RPM=",".join(str(r) for r in rpms))
    if setting:
        env["BAL_SET"] = setting
    if sim:
        env["BAL_SIM"] = sim
    res = subprocess.run([probe, "[.balance]"], env=env, capture_output=True, text=True)
    books, cur = [], None
    for line in res.stdout.splitlines():
        f = line.split()
        if not f:
            continue
        if f[0] == "begin":
            cur = {}
        elif f[0] == "end" and cur is not None:
            books.append(cur)
            cur = None
        elif cur is not None and len(f) == 2:
            cur[f[0]] = float(f[1])
    if len(books) != len(rpms):
        sys.exit(f"the probe failed on {slug}:\n{res.stdout[-2000:]}\n{res.stderr[-2000:]}")
    return books


def mark(v, rng):
    if rng is None:
        return f"{v:.3f}", ""
    return (f"**{v:.3f}**", "out") if not (rng[0] <= v <= rng[1]) else (f"{v:.3f}", "")


def section(slug, books):
    md = [f"### `{slug}`", ""]
    hdr = " | ".join(f"{b['rpm']:.0f} rpm" for b in books)
    md += [f"| | {hdr} |", "|---" * (len(books) + 1) + "|"]
    md.append("| Torque, N m | " + " | ".join(f"{b['torque']:.1f}" for b in books) + " |")
    md.append("| BMEP, bar | " + " | ".join(f"{b['bmep_tel'] / 1e5:.2f}" for b in books) + " |")
    md.append("| Lambda | " + " | ".join(f"{b['lambda']:.3f}" for b in books) + " |")
    md += ["", "Air, g per cycle per litre (and over the throttle's):", "",
           f"| | {hdr} |", "|---" * (len(books) + 1) + "|"]
    for key, label in AIR:
        md.append(f"| {label} | " + " | ".join(
            f"{b[key]:.4f} ({b[key] / b['air_throttle']:.3f})" for b in books) + " |")
    md.append("| VE: air / manifold (tel) | " + " | ".join(
        f"{b['ve_air_tel']:.3f} / {b['ve_man_tel']:.3f}" for b in books) + " |")
    md.append("| Air outside / heated at the port, K | " + " | ".join(
        f"{b['t_amb']:.0f} / {b['t_air_heated']:.0f}" for b in books) + " |")
    md += ["", "Energy, shares of the fuel's lower heating value:", "",
           f"| | {hdr} | Range | |", "|---" * (len(books) + 3) + "|"]
    for key, label, rng, _ in ENERGY:
        cells, outs = [], []
        for b in books:
            c, o = mark(b[key], rng)
            cells.append(c)
            outs.append(o)
        r = f"{rng[0]:.3f} to {rng[1]:.3f}" if rng else "(reported)"
        md.append(f"| {label} | " + " | ".join(cells) + f" | {r} | {'out' if any(outs) else ''} |")
    md.append("")
    return md


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default="")
    ap.add_argument("--only", default="")
    ap.add_argument("--out", default=os.path.join(ROOT, "docs", "measurements", "calibration", "balance", "balance.md"))
    ap.add_argument("--set", default="")
    ap.add_argument("--sim", default="")
    a = ap.parse_args()
    probe = find_tool("tee-tests-calibration-balance", a.bin)
    slugs = a.only.split(",") if a.only else SEVEN
    commit = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, capture_output=True, text=True).stdout.strip()
    md = ["# The air and energy balance", "",
          f"*Generated by `scripts/balance.py` (card N3): do not edit by hand. Commit {commit} (builds loaded whole, tee::Build)"
          + (f", with `{a.set}`" if a.set else "") + ". The engine dyno at full throttle, warm, each speed held 2 s with "
          "the exhaust's and the intake's heat settled as tee-dyno --steps settles them, then measured over 1 s; the "
          "speeds are the middle of the targets' speeds at peak torque and peak power. **Bold**: outside the range.*", ""]
    for slug in slugs:
        pk = peaks(slug)
        rpms = [pk[k] for k in ("torque", "power") if k in pk]
        md += section(slug, run(probe, slug, rpms, a.set, a.sim))
    md += ["### The ranges' sources", ""] + [f"- **{label}**: {src}" for _, label, rng, src in ENERGY if rng]
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    open(a.out, "w").write("\n".join(md) + "\n")
    print(a.out)


if __name__ == "__main__":
    main()
