#!/usr/bin/env python3
"""tests/sim/bike/make_builds.py: card B2's motorcycle builds, written from the tables below.

Each engine is a custom layout (spec.engine.custom, docs/models/mechanics.md): its cylinders' banks, crank pins and
firing revolutions are worked out here from the firing angles wanted (fire = ((bankAngle - pin) mod 360) + 360 fireRev,
core/layout.fm), so the file and the arithmetic cannot disagree; the "expect" key (kept by the host as an unknown key)
holds the order and intervals the tests check against. A whole bike is an engine plus a car section (kind Motorcycle,
model 1, card B2's fields).

    python3 tests/sim/bike/make_builds.py          # rewrites content/builds/engines/bike-*.json and content/builds/bike-*.json
"""
import json
import math
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

# name, title, notes, base layout (its pipes, idle and crank are the reference's for it), bank angles, each cylinder's
# bank and its firing angle after cylinder 1's (degrees), bore and stroke (mm), compression, redline (rpm), crank and
# flywheel (kg m^2, 0: the layout's), rod (mm, 0: 3.3 crank radii), spacing along the crank (m)
ENGINES = [
    ("bike-i4-screamer", "Inline four, 180-degree crank (a screamer)",
     "A superbike's litre four, flat-plane: 1-2-4-3, a firing every 180 degrees. 76 x 55 mm (998 cc), 13.0:1, redline "
     "14,500 rpm: a 2010s litre superbike's numbers in round figures, not one model's.",
     "Crossplane I4", [0.0], [0, 0, 0, 0], [0, 180, 540, 360], 76.0, 55.0, 13.0, 14500, 0.0, 0.0, 0.085),
    ("bike-i4-crossplane", "Inline four, crossplane crank (270-180-90-180)",
     "Pins at 0, 270, 90 and 180 degrees: 1-3-2-4 at 270-180-90-180, the uneven beat of a crossplane superbike four. "
     "79 x 50.9 mm (998 cc), 13.0:1, redline 14,000.",
     "Crossplane I4", [0.0], [0, 0, 0, 0], [0, 450, 270, 540], 79.0, 50.9, 13.0, 14000, 0.0, 0.0, 0.085),
    ("bike-i4-600", "Inline four, 600 cc, 180-degree crank",
     "A supersport 600's four: 67 x 42.5 mm (599 cc), 13.1:1, redline 16,000 rpm: the highest-revving engine here.",
     "Crossplane I4", [0.0], [0, 0, 0, 0], [0, 180, 540, 360], 67.0, 42.5, 13.1, 16000, 0.0, 0.0, 0.075),
    ("bike-vtwin-90", "V-twin, 90 degrees, one pin",
     "An L-twin's 270-450 (a 90-degree V on one pin): 100 x 70 mm (1099 cc), 12.5:1, redline 11,000.",
     "Twin 270", [45.0, -45.0], [0, 1], [0, 270], 100.0, 70.0, 12.5, 11000, 0.0, 0.0, 0.03),
    ("bike-vtwin-45", "V-twin, 45 degrees, one pin (a cruiser's)",
     "The big cruiser twin's 315-405 potato-potato: 102 x 114 mm (1863 cc), 10.0:1, redline 5,500, a heavy flywheel.",
     "V-twin 45", [22.5, -22.5], [0, 1], [0, 315], 102.0, 114.0, 10.0, 5500, 0.0, 0.0, 0.0),
    ("bike-twin-180", "Parallel twin, 180-degree crank",
     "Pins opposed: 180-540. 83 x 68.6 mm (742 cc), 11.5:1, redline 10,500.",
     "Twin 270", [0.0], [0, 0], [0, 180], 83.0, 68.6, 11.5, 10500, 0.0, 0.0, 0.09),
    ("bike-twin-270", "Parallel twin, 270-degree crank",
     "A V-twin's beat from a parallel twin: 270-450. 80 x 68.6 mm (689 cc), 11.5:1, redline 10,000.",
     "Twin 270", [0.0], [0, 0], [0, 270], 80.0, 68.6, 11.5, 10000, 0.0, 0.0, 0.09),
    ("bike-twin-360", "Parallel twin, 360-degree crank",
     "Both pistons together, firing alternately: 360-360, the old British twin's beat. 76 x 71 mm (644 cc), 9.5:1, "
     "redline 8,000.",
     "Twin 360", [0.0], [0, 0], [0, 360], 76.0, 71.0, 9.5, 8000, 0.0, 0.0, 0.09),
    ("bike-triple", "Inline three, 120-degree crank",
     "Even firing every 240 degrees, 1-2-3: 78 x 53.4 mm (765 cc), 12.9:1, redline 13,000.",
     "Crossplane I4", [0.0], [0, 0, 0], [0, 240, 480], 78.0, 53.4, 12.9, 13000, 0.0, 0.0, 0.085),
    ("bike-v4-screamer", "V4, 90 degrees, even firing (a screamer)",
     "Pins at 45 and 135 degrees, each shared by a front and a rear cylinder: a firing every 180 degrees. 81 x 48.4 mm "
     "(998 cc), 13.6:1, redline 14,500.",
     "Crossplane I4", [45.0, -45.0], [0, 1, 0, 1], [0, 180, 360, 540], 81.0, 48.4, 13.6, 14500, 0.0, 0.0, 0.035),
    ("bike-v4-bigbang", "V4, 90 degrees, big bang (twin pulse)",
     "Pins offset 70 degrees: 0-90-290-380, two pairs of firings close together, the race V4's twin pulse. 81 x 53.5 mm "
     "(1103 cc), 14.0:1, redline 13,000.",
     "Crossplane I4", [45.0, -45.0], [0, 1, 0, 1], [0, 90, 290, 380], 81.0, 53.5, 14.0, 13000, 0.0, 0.0, 0.035),
    ("bike-boxer-twin", "Boxer twin, 360 degrees",
     "Opposed cylinders on pins 180 apart: both at top dead centre together, firing alternately, 360-360. 102.5 x 76 mm "
     "(1254 cc), 12.5:1, redline 9,000.",
     "Twin 360", [90.0, -90.0], [0, 1], [0, 360], 102.5, 76.0, 12.5, 9000, 0.0, 0.0, 0.07),
    ("bike-single", "Single",
     "A road single: 89 x 60 mm (373 cc), 12.6:1, redline 10,500.",
     "Single", [0.0], [0], [0], 89.0, 60.0, 12.6, 10500, 0.0, 0.0, 0.0),
]

# Each engine's head and cams for card E's valvetrain lab (valve.model Lab) and its runner (intake.model Lab, a body a
# runner): valves a cylinder, valve diameters (mm), duration at 1 mm (deg), lift (mm) and centreline (deg) for the
# intake and the exhaust, the train, the runner's length (m) for its tuned speed. Judgement in the class's ranges: a
# sport four's intake valves are some 0.38 to 0.40 of its bore (two a cylinder: 0.30 of the bore's area), 250 to 270
# degrees at 1 mm, 9 to 11 mm of lift on finger followers; a cruiser's pushrod twin 220 degrees and 12 mm.
HEADS = {
    "bike-i4-screamer":   (4, 31.0, 25.0, 262, 10.5, 104, 252, 9.0, 106, "Finger", 0.26),
    "bike-i4-crossplane": (4, 31.0, 25.0, 262, 10.5, 104, 252, 9.0, 106, "Finger", 0.26),
    "bike-i4-600":        (4, 27.0, 22.0, 266, 9.3, 104, 256, 8.0, 106, "Finger", 0.23),
    "bike-vtwin-90":      (4, 42.0, 34.0, 250, 11.5, 106, 244, 10.5, 108, "Rocker", 0.32),
    "bike-vtwin-45":      (4, 40.0, 33.0, 220, 12.0, 108, 220, 11.0, 110, "Pushrod", 0.42),
    "bike-twin-180":      (4, 32.0, 27.0, 240, 9.5, 106, 236, 8.5, 108, "Bucket", 0.32),
    "bike-twin-270":      (4, 31.5, 26.5, 236, 9.0, 106, 232, 8.0, 108, "Bucket", 0.34),
    "bike-twin-360":      (2, 38.0, 32.0, 228, 9.0, 108, 228, 8.5, 108, "Rocker", 0.36),
    "bike-triple":        (4, 31.0, 25.5, 256, 10.0, 104, 248, 9.0, 106, "Finger", 0.27),
    "bike-v4-screamer":   (4, 34.0, 27.5, 264, 11.0, 104, 254, 9.5, 106, "Finger", 0.25),
    "bike-v4-bigbang":    (4, 34.0, 27.5, 262, 11.0, 104, 252, 9.5, 106, "Finger", 0.26),
    "bike-boxer-twin":    (4, 40.0, 34.0, 244, 10.5, 106, 240, 9.5, 108, "Finger", 0.34),
    "bike-single":        (4, 34.0, 28.0, 244, 9.5, 106, 238, 8.5, 108, "Bucket", 0.32),
}

AIR = {"temperature": 300.0, "pressure": 101325}


def hardware(name):
    v, di, de, idur, il, ic, edur, el, ec, train, runner = HEADS[name]
    valve = {"model": "Lab", "valves": v, "dIn": di / 1000, "dEx": de / 1000,
             "inDur1mm": idur, "inDurSeat": idur + 40, "inLift": il / 1000, "inCentre": ic,
             "exDur1mm": edur, "exDurSeat": edur + 40, "exLift": el / 1000, "exCentre": ec, "train": train}
    intake = {"model": "Lab", "throttleKind": "A body a runner", "trumpets": "The air box", "runnerLength": runner}
    return valve, intake


def geometry(banks, bank_of, fire):
    """The pins and firing revolutions that put each cylinder's firing where `fire` says (cylinder 1 at its bank's
    angle: pin = that angle, so its top dead centre is at crank angle 0)."""
    pins, revs = [], []
    a0 = banks[bank_of[0]]
    for c, f in enumerate(fire):
        a = banks[bank_of[c]]
        # fire_abs(c) = ((a - pin) mod 360) + 360 rev must equal f (cylinder 1's own is 0 with pin = a0)
        rev = 1 if f >= 360 else 0
        pin = (a - (f % 360)) % 360 if c > 0 else a0 % 360
        pins.append(round(pin, 4))
        revs.append(rev)
    return pins, revs


def firing(banks, bank_of, pins, revs):
    """core/layout.fm's arithmetic, again: the firing angles after cylinder 1's, the order, the intervals"""
    def wrap(x, m):
        return x - m * math.floor(x / m)
    raw = [wrap(banks[bank_of[c]] - pins[c], 360) + 360 * revs[c] for c in range(len(pins))]
    rel = [wrap(r - raw[0], 720) for r in raw]
    order = sorted(range(len(rel)), key=lambda c: rel[c])
    angles = [rel[c] for c in order]
    intervals = [round(((angles[(i + 1) % len(angles)] - angles[i]) % 720) or 720, 4) for i in range(len(angles))]
    return [round(x, 4) for x in rel], [c + 1 for c in order], intervals


def engine_spec(e):
    name, title, notes, base, banks, bank_of, fire, bore, stroke, cr, red, jfly, rod, pitch = e
    n = len(bank_of)
    pins, revs = geometry(banks, bank_of, fire)
    rel, order, intervals = firing(banks, bank_of, pins, revs)
    assert all(abs(rel[c] - fire[c]) < 1e-6 for c in range(n)), (name, rel, fire)
    disp = n * math.pi / 4 * (bore / 1000) ** 2 * (stroke / 1000)
    places = []
    for c in range(n):
        # a V or a boxer: each pair of cylinders on its pin side by side; an inline engine evenly spaced
        places.append(round(pitch * (c // 2 if len(banks) > 1 else c), 4))
    spec = {
        "layout": base,
        "displacement": round(disp, 7),
        "compression": cr,
        "custom": True,
        "cylinders": n,
        "bankAngle": banks,
        "bank": bank_of,
        "pin": pins,
        "fireRev": revs,
        "place": places,
        "bore": bore / 1000,
        "stroke": stroke / 1000,
        "redlineSpeed": round(red * math.pi / 30, 3),
    }
    if jfly > 0:
        spec["flywheelInertia"] = jfly
    if rod > 0:
        spec["rodLength"] = rod / 1000
    expect = {"firingOrder": order, "firingIntervals": intervals, "firingAngles": sorted(rel),
              "redlineRpm": red, "displacementCc": round(disp * 1e6, 1)}
    return spec, expect


# The whole bikes: a title, notes, the engine, the car section's fields (the bike), and figures for the tests
# (`expect.class`: what such a bike does, with sources; judgement where it says so).
BIKES = [
    ("bike-superbike", "Superbike (litre four, crossplane)",
     "A litre superbike: 201 kg wet, 1.41 m, an 80 kg rider, hypersport tyres, a quickshifter and an auto-blipper, a "
     "slipper clutch, wheelie control. The crossplane four. A class's numbers in round figures, not one model's.",
     "bike-i4-crossplane",
     {"mass": 201, "wheelbase": 1.41, "cgHeight": 0.56, "frontWeight": 0.53, "cdA": 0.50, "tyreRadius": 0.31,
      "wheelInertia": 0.8, "brakeDecel": 1.2, "brakeBias": 0.5, "gears": 6, "autoGearing": False,
      "ratio": [2.60, 2.00, 1.67, 1.44, 1.30, 1.19], "gearEfficiency": 0.97,
      "riderMass": 80, "riderHeight": 1.0, "riderForward": 0.42, "tuck": 1.0, "cdATuck": 0.30, "pitchInertia": 50,
      "bikeTyre": "Hypersport", "primaryRatio": 1.63, "frontSprocket": 16, "rearSprocket": 43,
      "chainEfficiency": 0.97, "chainLash": 0.035, "slipper": True, "slipperTorque": 0.15, "quickshifter": True,
      "quickshiftCut": 0.05, "autoBlip": True, "wheelieControl": "By the throttle", "wheelieAngle": 4}),
    ("bike-supersport-600", "Supersport 600",
     "A 600 cc supersport: 190 kg wet, 1.38 m, an 80 kg rider, sport tyres, a quickshifter, a slipper clutch. Its four "
     "screams to 16,000 rpm.",
     "bike-i4-600",
     {"mass": 190, "wheelbase": 1.38, "cgHeight": 0.55, "frontWeight": 0.53, "cdA": 0.50, "tyreRadius": 0.31,
      "wheelInertia": 0.75, "brakeDecel": 1.15, "brakeBias": 0.5, "gears": 6, "autoGearing": False,
      "ratio": [2.67, 2.06, 1.75, 1.53, 1.39, 1.28], "gearEfficiency": 0.97,
      "riderMass": 80, "riderHeight": 1.0, "riderForward": 0.42, "tuck": 1.0, "cdATuck": 0.31, "pitchInertia": 48,
      "bikeTyre": "Sport", "primaryRatio": 2.07, "frontSprocket": 16, "rearSprocket": 44,
      "chainEfficiency": 0.97, "chainLash": 0.035, "slipper": True, "slipperTorque": 0.15, "quickshifter": True,
      "quickshiftCut": 0.05, "autoBlip": False, "wheelieControl": "By the throttle", "wheelieAngle": 4}),
    ("bike-naked-twin", "Naked twin (parallel, 270)",
     "A middleweight naked: 184 kg wet, 1.40 m, an 80 kg rider sitting up, sport tyres, a 270-degree parallel twin of "
     "689 cc. No quickshifter, no slipper.",
     "bike-twin-270",
     {"mass": 184, "wheelbase": 1.40, "cgHeight": 0.56, "frontWeight": 0.50, "cdA": 0.58, "tyreRadius": 0.31,
      "wheelInertia": 0.75, "brakeDecel": 1.1, "brakeBias": 0.5, "gears": 6, "autoGearing": False,
      "ratio": [2.85, 2.06, 1.64, 1.38, 1.21, 1.08], "gearEfficiency": 0.97,
      "riderMass": 80, "riderHeight": 1.05, "riderForward": 0.40, "tuck": 0.3, "cdATuck": 0.40, "pitchInertia": 45,
      "bikeTyre": "Sport", "primaryRatio": 1.9, "frontSprocket": 16, "rearSprocket": 43,
      "chainEfficiency": 0.97, "chainLash": 0.04, "slipper": False, "quickshifter": False, "autoBlip": False,
      "wheelieControl": "By the throttle", "wheelieAngle": 6}),
    ("bike-cruiser", "Cruiser (45-degree V-twin)",
     "A big cruiser: 320 kg wet, 1.67 m, an 80 kg rider sitting up, cruiser tyres, a 1863 cc 45-degree twin, a belt of "
     "30 to 70 teeth. No quickshifter, no slipper.",
     "bike-vtwin-45",
     {"mass": 320, "wheelbase": 1.67, "cgHeight": 0.52, "frontWeight": 0.45, "cdA": 0.75, "tyreRadius": 0.33,
      "wheelInertia": 1.1, "brakeDecel": 1.0, "brakeBias": 0.4, "gears": 6, "autoGearing": False,
      "ratio": [3.34, 2.30, 1.71, 1.35, 1.12, 0.89], "gearEfficiency": 0.97,
      "riderMass": 80, "riderHeight": 0.95, "riderForward": 0.40, "tuck": 0.0, "cdATuck": 0.65, "pitchInertia": 80,
      "bikeTyre": "Cruiser", "primaryRatio": 1.45, "frontSprocket": 30, "rearSprocket": 70,
      "chainEfficiency": 0.96, "chainLash": 0.03, "slipper": False, "quickshifter": False, "autoBlip": False,
      "wheelieControl": "Off", "wheelieAngle": 4}),
    ("bike-adventure-twin", "Adventure twin (boxer)",
     "A big adventure bike: 249 kg wet, 1.52 m, an 80 kg rider sitting up, dual-sport tyres, a 1254 cc boxer twin; a "
     "shaft in the real one, a chain's numbers here. A quickshifter both ways.",
     "bike-boxer-twin",
     {"mass": 249, "wheelbase": 1.52, "cgHeight": 0.60, "frontWeight": 0.50, "cdA": 0.70, "tyreRadius": 0.33,
      "wheelInertia": 1.0, "brakeDecel": 1.1, "brakeBias": 0.45, "gears": 6, "autoGearing": False,
      "ratio": [2.44, 1.73, 1.39, 1.16, 1.00, 0.87], "gearEfficiency": 0.97,
      "riderMass": 80, "riderHeight": 1.1, "riderForward": 0.40, "tuck": 0.0, "cdATuck": 0.55, "pitchInertia": 65,
      "bikeTyre": "Dual sport", "primaryRatio": 1.65, "frontSprocket": 17, "rearSprocket": 47,
      "chainEfficiency": 0.95, "chainLash": 0.03, "slipper": True, "slipperTorque": 0.2, "quickshifter": True,
      "quickshiftCut": 0.06, "autoBlip": True, "wheelieControl": "By the throttle", "wheelieAngle": 4}),
    ("bike-small-single", "Small single",
     "A light naked single: 172 kg wet, 1.36 m, an 80 kg rider sitting up, sport tyres, 373 cc. A slipper clutch.",
     "bike-single",
     {"mass": 172, "wheelbase": 1.36, "cgHeight": 0.54, "frontWeight": 0.50, "cdA": 0.55, "tyreRadius": 0.30,
      "wheelInertia": 0.6, "brakeDecel": 1.0, "brakeBias": 0.5, "gears": 6, "autoGearing": False,
      "ratio": [2.67, 1.88, 1.43, 1.18, 1.00, 0.88], "gearEfficiency": 0.97,
      "riderMass": 80, "riderHeight": 1.05, "riderForward": 0.40, "tuck": 0.2, "cdATuck": 0.42, "pitchInertia": 40,
      "bikeTyre": "Sport", "primaryRatio": 2.97, "frontSprocket": 15, "rearSprocket": 45,
      "chainEfficiency": 0.97, "chainLash": 0.04, "slipper": True, "slipperTorque": 0.2, "quickshifter": False,
      "autoBlip": False, "wheelieControl": "By the throttle", "wheelieAngle": 4}),
]


def write(path, doc):
    with open(path, "w") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")


def main():
    engines = {}
    for e in ENGINES:
        spec, expect = engine_spec(e)
        engines[e[0]] = (spec, expect)
        valve, intake = hardware(e[0])
        doc = {"format": "tee-build", "version": 1, "name": e[1], "notes": e[2],
               "spec": {"engine": spec, "valve": valve, "intake": intake, "env": AIR}, "expect": expect}
        write(os.path.join(ROOT, "content", "builds", "engines", e[0] + ".json"), doc)
        print(f"{e[0]:22s} order {expect['firingOrder']} intervals {expect['firingIntervals']} "
              f"{expect['displacementCc']:.0f} cc, {expect['redlineRpm']} rpm")
    for name, title, notes, eng, car in BIKES:
        spec, expect = engines[eng]
        valve, intake = hardware(eng)
        c = {"kind": "Motorcycle", "model": "Vehicle"}
        c.update(car)
        doc = {"format": "tee-build", "version": 1, "name": title, "notes": notes,
               "spec": {"engine": spec, "valve": valve, "intake": intake,
                        # a short system: short headers, a short pipe, a small can (card G's builder's 4-2-1 when it
                        # reaches the engine)
                        "exhaust": {"headers": 0.25, "pipe": 0.2, "muffler": 0.15},
                        "fuel": {"kind": "Gasoline"}, "env": AIR, "car": c, "sound": {"view": "Exhaust"}},
               "expect": {"engine": eng}}
        write(os.path.join(ROOT, "content", "builds", name + ".json"), doc)
        print(f"{name:22s} with {eng}")


if __name__ == "__main__":
    main()
