#!/usr/bin/env python3
"""One sheet per archetype for card N3 (the naturally aspirated over-delivery): the build's hardware beside the real
engine's published figures, the target's basis (crank, net, which variant) and table F's measurement.

    python3 content/validation/archetypes/sheets/make.py   writes na-over-delivery.md beside it
"""
import json
import math
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
RPM = math.pi / 30

# The real engine each archetype stands for, as published. MEMORY marks a figure recalled, not looked up today
# (firm to the digit given unless said); SEARCH a figure confirmed by a web search this session.
REAL = {
    "air-cooled-flat-six-36": dict(
        engine="the 1989-93 3.6 L air-cooled flat six (M64/01, 964 Carrera 2/4)",
        basis="184 kW (250 PS DIN) at 6100 rpm, 310 N m at 4800: DIN net at the crank, the European rating (SEARCH: "
              "Wikipedia, auto-data.net); the US SAE net rating is 247 hp (184 kW): the same",
        bore="100 x 76.4 mm, 3600 cc, 11.3:1 (SEARCH)",
        head="two valves a cylinder, one cam a bank by chain, rocker arms, twin spark plugs (MEMORY)",
        valves="intake 49 mm, exhaust 42.5 mm (MEMORY, to 1 mm)",
        cam="mild road cam; published duration not to hand (MEMORY: about 240 deg at the seat)",
        intake="a resonance intake with a switching flap (the 964's two-stage plenum) (MEMORY)",
        other="dry sump; air cooled: the head and barrels run hotter than a water-cooled engine's"),
    "vr6-28": dict(
        engine="the 1991-2000 2.8 L 12-valve narrow-angle VR6 (AAA)",
        basis="128 kW (174 PS DIN) at 5800 rpm, 235 N m at 4200: DIN net at the crank (MEMORY, firm)",
        bore="81 x 90.3 mm, 2792 cc, 10.0:1 (MEMORY, firm)",
        head="one head over both banks, two valves a cylinder, two cams (one for each row of valves), rocker followers (MEMORY)",
        valves="intake about 40 mm, exhaust about 35 mm (MEMORY, to 2 mm)",
        cam="mild road cam (MEMORY: under 230 deg at the seat)",
        intake="a two-stage variable-length intake manifold on the later cars; the long runners for torque (MEMORY)",
        other="its ports are long and unequal (the narrow vee): a known breathing limit"),
    "twin-cam-roadster-18": dict(
        engine="the 1.8 L 16-valve twin-cam four of a light roadster (BP type, 1994-2005)",
        basis="96 to 107 kW at 6500 to 7000 rpm, 155 to 170 N m at 5000 to 5500: DIN/SAE net at the crank across its "
              "versions; the later (variable intake cam) one 107 kW / 168 N m (MEMORY, firm to 5 %)",
        bore="83 x 85 mm, 1839 cc, 9.0:1 to 10.0:1 by version (MEMORY)",
        head="four valves, two cams by belt, buckets (MEMORY)",
        valves="intake about 31 mm, exhaust about 26 mm (MEMORY, to 1 mm)",
        cam="mild: about 230 to 240 deg at the seat, about 8.5 to 9 mm lift (MEMORY)",
        intake="long runners on a plenum, tuned for the mid-range (MEMORY)",
        other="the build carries 33/28 mm valves and 244/240 deg cams: larger and longer than the real head's"),
    "small-block-57-carb": dict(
        engine="the 350 cu in small block of the 1970s-80s (L48 and its kin, four-barrel carburettor)",
        basis="the kind's SAE net ratings at the crank 120 to 170 kW (the L48 165 to 190 hp, the L82 205 to 210 hp), "
              "N m 340 to 400; the target's 140 to 210 kW and 380 to 470 N m reach the hottest variant (MEMORY)",
        bore="101.6 x 88.4 mm, 5733 cc, 8.2:1 to 9.0:1 in the net-rating years (MEMORY, firm)",
        head="two valves, pushrods, a flat-tappet hydraulic cam (MEMORY)",
        valves="intake 1.94 in (49.3 mm), exhaust 1.50 in (38.1 mm) (MEMORY, firm)",
        cam="the L48's about 195/202 deg at 0.050 in, 0.390/0.410 in lift (9.9/10.4 mm) (MEMORY): the build's 214/224 "
            "at 1 mm and 10.9 mm are nearer the L82's hotter cam",
        intake="a dual-plane manifold under a four-barrel, restrictive exhaust manifolds and a single catalytic converter (MEMORY)",
        other="the build's compression 9.0 is the top of the era's; its cam is hotter than the L48's"),
    "pushrod-v6-38": dict(
        engine="the 3.8 L 90-degree pushrod V6 of the 1990s (Series II, L36)",
        basis="153 kW (205 hp SAE net) at 5200 rpm, 312 N m (230 lb ft) at 4000: SAE net at the crank (MEMORY, firm)",
        bore="96.5 x 86.4 mm, 3791 cc, 9.4:1 (MEMORY, firm)",
        head="two valves, pushrods, a roller cam, a balance shaft (MEMORY)",
        valves="intake 1.84 in (46.7 mm), exhaust 1.50 in (38.1 mm) (MEMORY)",
        cam="mild, about 0.41 in (10.4 mm) lift (MEMORY)",
        intake="a tuned plastic upper intake on long runners, for torque (MEMORY)",
        other="the build's 46/38 mm valves and mild cam match"),
    "big-bore-70-pushrod-v8": dict(
        engine="the 7.0 L dry-sump small-block of 2006-13 (LS7)",
        basis="377 kW (505 hp SAE net) at 6300 rpm, 637 N m (470 lb ft) at 4800: SAE net at the crank (gmauthority.com; "
              "SEARCH for the bore and stroke)",
        bore="104.8 x 101.6 mm, 7011 cc, 11.0:1 (SEARCH)",
        head="two valves, pushrods, CNC-ported heads with raised runners, titanium intake valves, roller lifters (MEMORY)",
        valves="intake 2.20 in (55.9 mm), exhaust 1.61 in (40.9 mm) (MEMORY, firm)",
        cam="211/230 deg at 0.050 in, 0.591 in (15.0 mm) lift both, 121 deg lobe separation (MEMORY, firm): the build's "
            "intake 230 deg at 1 mm is some 15 deg longer than the real one's at 0.050 in",
        intake="a long-runner composite intake (MEMORY)",
        other="its heads flow about 320 to 330 CFM at 0.6 in (MEMORY): the best production two-valve heads"),
    "four-cam-v8-50": dict(
        engine="the 5.0 L four-cam V8 of 2018 on (third generation, direct and port injection)",
        basis="343 kW (460 hp SAE net) at 7000 rpm, 569 N m (420 lb ft) at 4600: SAE net at the crank (MEMORY, firm); "
              "the earlier ones 307 kW / 529 N m (2011) and 324 / 542 (2015)",
        bore="93 x 92.7 mm, 5038 cc, 12.0:1 (MEMORY, firm)",
        head="four valves, two cams a bank by chain, roller finger followers, all four cams phased (MEMORY)",
        valves="intake about 37.7 mm, exhaust about 31 mm (MEMORY, to 1 mm)",
        cam="about 260 deg at the seat, about 12 mm intake lift (MEMORY)",
        intake="a two-stage (charge motion) composite intake (MEMORY)",
        other="the build's 37/31 mm valves match"),
}


def figures(slug, doc):
    s = doc["spec"]
    e, v, i, x = s["engine"], s["valve"], s.get("intake", {}), s["exhaust"]
    out = {
        "bore": "%.1f x %.1f mm, %.0f cc, %.1f:1" % (e["bore"] * 1000, e["stroke"] * 1000, e["displacement"] * 1e6,
                                                   e["compression"]),
        "rod": "%.1f mm" % (e["rodLength"] * 1000),
        "redline": "%.0f rpm" % (e["redlineSpeed"] / RPM),
        "head": "%d valves, %s train, ports %s/%s (quality: 1.35/1.5 the default, a stock 232 CFM small-block head at "
                "1.35)" % (v["valves"], v["train"], v.get("portIn", 1.35), v.get("portEx", 1.5)),
        "valves": "intake %.1f mm, exhaust %.1f mm" % (v["dIn"] * 1000, v["dEx"] * 1000),
        "cam": "intake %d/%d deg (seat/1 mm), %.1f mm, centre %.0f; exhaust %d/%d, %.1f mm, centre %.0f%s" % (
            v["inDurSeat"], v["inDur1mm"], v["inLift"] * 1000, v["inCentre"], v["exDurSeat"], v["exDur1mm"],
            v["exLift"] * 1000, v["exCentre"],
            "; phasers %d/%d deg" % (v["inPhaseRange"], v["exPhaseRange"]) if v.get("inPhaseRange") else ""),
        "intake": "runner %.2f m%s; %s" % (i.get("runnerLength", 0.30),
                                         " / %.2f m above %.0f rpm" % (i["runnerLength2"], i["runnerSwitch"] / RPM)
                                         if i.get("runnerLength2") else "", i.get("throttleKind", "one throttle and a plenum")),
        "exhaust": "converter %s, mufflers %s, sides %s" % (x.get("converter"), x.get("mufKind"), x.get("sides")),
        "springs": "%.0f N/m over %.0f N" % (v.get("springRate", 0), v.get("springPreload", 0)),
    }
    return out


def measured(slug, table):
    for line in table.splitlines():
        if line.startswith("| ") and line.count("|") > 6:
            cells = [c.strip() for c in line.split("|")]
            yield cells


def main():
    table_path = os.path.join(ROOT, "docs", "manual", "validation.md")
    table = open(table_path).read()
    commit = re.search(r"Run at commit `([^`]+)`", table)
    out = ["# The naturally aspirated over-delivery: seven engines, build beside the real one",
           "",
           "*For card N3. Written by `content/validation/archetypes/sheets/make.py` from the builds "
           "(`content/builds/archetypes/`) and the newest table (`docs/manual/validation.md`, commit %s). The real "
           "engine's figures: SEARCH, confirmed by a web search on 2026-11-24 (the sources named); MEMORY, recalled and "
           "not looked up, firm as said. Every target is at the crank and net (SAE J1349 or DIN 70020: with the "
           "engine's own exhaust, intake and accessories), never at the wheels.*" % (commit.group(1) if commit else "?"),
           ""]
    for slug, real in REAL.items():
        doc = json.load(open(os.path.join(ROOT, "content", "builds", "archetypes", slug + ".json")))
        f = figures(slug, doc)
        row = next((c for c in measured(slug, table) if c[1] == doc["name"]), None)
        out += ["## %s (`%s`)" % (doc["name"], slug), "",
                "Stands for %s. **Target basis:** %s." % (real["engine"], real["basis"]), ""]
        if row:
            out += ["**Measured (the table):** power %s; torque %s." % (row[2], row[3]), ""]
        out += ["| | The build | The real engine |", "|---|---|---|",
                "| Bore, stroke, compression | %s (rod %s) | %s |" % (f["bore"], f["rod"], real["bore"]),
                "| Head | %s | %s |" % (f["head"], real["head"]),
                "| Valves | %s | %s |" % (f["valves"], real["valves"]),
                "| Cams | %s | %s |" % (f["cam"], real["cam"]),
                "| Intake | %s | %s |" % (f["intake"], real["intake"]),
                "| Exhaust | %s | (a road system with converters) |" % f["exhaust"],
                "| Springs, redline | %s; %s | |" % (f["springs"], f["redline"]),
                "", "*Note:* " + real["other"] + ".", ""]
    with open(os.path.join(HERE, "na-over-delivery.md"), "w") as fh:
        fh.write("\n".join(out))
    print("wrote na-over-delivery.md")


if __name__ == "__main__":
    main()
