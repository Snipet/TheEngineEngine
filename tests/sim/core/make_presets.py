#!/usr/bin/env python3
# tests/sim/core/make_presets.py: writes content/builds/engines/*.json, the layout presets, from the geometry below.
#
# Each engine is written as geometry (each cylinder's bank, crank pin angle, firing revolution and place along the
# crank; each bank's angle) and, separately, as what it is known to do (its firing order and intervals, from the
# engine's published firing order and crank). This script derives the firing angles from the geometry the way
# sim/core/layout.fm does, and refuses to write a file whose geometry does not give what the engine is known to do.
# The C++ tests (tests/sim/core/LayoutTest.cpp) then check FMinus's derivation against the file's "expect".
#
#   python3 tests/sim/core/make_presets.py          # rewrites the presets; prints each one's intervals
#
# Conventions (sim/core/layout.fm): angles in crank degrees, in the crank's direction of rotation. A bank's angle is
# its cylinders' axis from vertical. A pin's angle is where the crank pin sits on the crank at crank angle 0. A
# cylinder is at top dead centre when the crank is at its bank's angle less its pin's; it fires at that top dead
# centre on the first revolution (fireRev 0) or the second (1). Places are metres along the crank from its front.
import json, os, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.join(ROOT, "content", "builds", "engines")
PITCH = 0.10   # metres between neighbouring crank throws (judgement: a bore of 85 to 100 mm and its webs)

def derive(e):
    """Firing angles (degrees after cylinder 1, 0..cycle), as sim/core/layout.fm derives them."""
    if e.get("rotors", 0) > 0:
        fa = []
        for r in range(e["rotors"]):
            tdc = (e["bankAngle"][0] - e["pin"][r]) % 360.0
            for f in range(3):
                fa.append(tdc + 360.0 * f)
        cyc = 1080.0
    else:
        fa = []
        for c in range(len(e["pin"])):
            tdc = (e["bankAngle"][e["bank"][c]] - e["pin"][c]) % 360.0
            fa.append(tdc + 360.0 * e["rev"][c])
        cyc = 720.0
    f0 = fa[0]
    return [round((a - f0) % cyc, 4) for a in fa], cyc

def order_intervals(fire, cyc):
    idx = sorted(range(len(fire)), key=lambda i: (fire[i], i))
    k = idx.index(0)
    idx = idx[k:] + idx[:k]
    iv = []
    for j in range(len(idx)):
        a = fire[idx[j]]
        b = fire[idx[(j + 1) % len(idx)]]
        d = (b - a) % cyc
        if len(idx) == 1:
            d = cyc
        iv.append(round(d, 4))
    return [i + 1 for i in idx], iv

def inline(pins, revs, bankAngle=0.0):
    n = len(pins)
    return dict(bank=[0] * n, pin=pins, rev=revs, place=[round(PITCH * i, 3) for i in range(n)], bankAngle=[bankAngle])

def vee(banks, pins, revs, bankAngles, throws):
    # throws: each cylinder's crank throw (0, 1, ...); a V's two cylinders on one throw sit a third of a pitch apart
    place = [round(PITCH * t + (PITCH / 3.0 if b % 2 else 0.0), 3) for t, b in zip(throws, banks)]
    return dict(bank=banks, pin=pins, rev=revs, place=place, bankAngle=bankAngles)

E = []
def add(name, title, notes, geo, order, intervals, ref=None, rotors=0, bankIntervals=None):
    e = dict(name=name, title=title, notes=notes, order=order, intervals=intervals, ref=ref, rotors=rotors,
             bankIntervals=bankIntervals)
    e.update(geo)
    E.append(e)

# ---- the reference's seventeen (sim/ref/layouts.fm), in its order ---------------------------------------------------
add("ref-00-single", "Single", "A thumper.", inline([0], [0]), [1], [720], ref=0)
add("ref-01-twin-360", "Parallel twin, 360-degree crank", "Both pins together; even firing.",
    inline([0, 0], [0, 1]), [1, 2], [360, 360], ref=1)
add("ref-02-twin-270", "Parallel twin, 270-degree crank", "Pins 90 degrees apart: a 90-degree V-twin's firing.",
    inline([0, 90], [0, 0]), [1, 2], [270, 450], ref=2)
add("ref-03-vtwin-45", "V-twin, 45 degrees, one pin", "The rear cylinder 45 degrees behind the front on a shared pin.",
    vee([0, 1], [22.5, 22.5], [0, 0], [22.5, -22.5], [0, 0]), [1, 2], [315, 405], ref=3)
add("ref-04-inline-3", "Inline three, 120-degree crank", "1-2-3, even.",
    inline([0, 120, 240], [0, 0, 1]), [1, 2, 3], [240, 240, 240], ref=4)
add("ref-05-inline-4", "Inline four, flat-plane crank", "1-3-4-2, even.",
    inline([0, 180, 180, 0], [0, 1, 0, 1]), [1, 3, 4, 2], [180, 180, 180, 180], ref=5)
add("ref-06-flat-4", "Boxer four", "1-3-2-4: cylinders 1 and 3 on the right bank, 2 and 4 on the left; each its own throw.",
    dict(bank=[0, 1, 0, 1], pin=[90, 270, 270, 90], rev=[0, 1, 0, 1], place=[0.0, 0.1, 0.2, 0.3], bankAngle=[90, -90]),
    [1, 3, 2, 4], [180, 180, 180, 180], ref=6)
add("ref-07-xplane-i4", "Inline four, crossplane crank", "1-3-2-4, pins 0, 270, 90, 180: 270-180-90-180.",
    inline([0, 270, 90, 180], [0, 1, 0, 1]), [1, 3, 2, 4], [270, 180, 90, 180], ref=7)
add("ref-08-inline-5", "Inline five", "1-2-4-5-3, even 144.",
    inline([0, 216, 144, 72, 288], [0, 0, 1, 0, 1]), [1, 2, 4, 5, 3], [144] * 5, ref=8)
add("ref-09-v6-60", "V6, 60 degrees, split pins", "1-2-3-4-5-6, odd cylinders left; six pins 60 degrees apart: even 120.",
    vee([0, 1, 0, 1, 0, 1], [330, 270, 90, 30, 210, 150], [0, 0, 0, 1, 1, 1], [-30, 30], [0, 0, 1, 1, 2, 2]),
    [1, 2, 3, 4, 5, 6], [120] * 6, ref=9, bankIntervals=[[240, 240, 240], [240, 240, 240]])
add("ref-10-inline-6", "Inline six", "1-5-3-6-2-4, even 120.",
    inline([0, 240, 120, 120, 240, 0], [0, 1, 0, 1, 0, 1]), [1, 5, 3, 6, 2, 4], [120] * 6, ref=10)
add("ref-11-flat-6", "Boxer six", "1-6-2-4-3-5; cylinders 1-3 right, 4-6 left; opposed pairs 180 degrees apart.",
    dict(bank=[0, 0, 0, 1, 1, 1], pin=[90, 210, 330, 270, 30, 150], rev=[0, 0, 1, 1, 1, 0],
         place=[0.0, 0.12, 0.24, 0.05, 0.17, 0.29], bankAngle=[90, -90]),
    [1, 6, 2, 4, 3, 5], [120] * 6, ref=11, bankIntervals=[[240, 240, 240], [240, 240, 240]])
add("ref-12-v8-cross", "V8, 90 degrees, crossplane", "1-8-4-3-6-5-7-2, odd cylinders left, pins shared 1-2, 3-4, 5-6, 7-8.",
    vee([0, 1, 0, 1, 0, 1, 0, 1], [45, 45, 135, 135, 315, 315, 225, 225], [0, 1, 0, 0, 1, 1, 1, 0], [45, -45],
        [0, 0, 1, 1, 2, 2, 3, 3]),
    [1, 8, 4, 3, 6, 5, 7, 2], [90] * 8, ref=12, bankIntervals=[[270, 180, 90, 180], [90, 180, 270, 180]])
add("ref-13-v8-flat", "V8, 90 degrees, flat-plane", "1-5-3-7-4-8-2-6, cylinders 1-4 one bank, 5-8 the other.",
    vee([0, 0, 0, 0, 1, 1, 1, 1], [315, 135, 135, 315, 315, 135, 135, 315], [0, 1, 0, 1, 0, 1, 0, 1], [-45, 45],
        [0, 1, 2, 3, 0, 1, 2, 3]),
    [1, 5, 3, 7, 4, 8, 2, 6], [90] * 8, ref=13, bankIntervals=[[180] * 4, [180] * 4])
add("ref-14-v10-72", "V10, 72 degrees, shared pins", "1-6-5-10-2-7-3-8-4-9, even 72; cylinders 1-5 one bank.",
    vee([0] * 5 + [1] * 5, [324, 36, 252, 108, 180, 324, 36, 252, 108, 180], [0, 0, 1, 1, 0, 0, 1, 1, 1, 0],
        [-36, 36], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4]),
    [1, 6, 5, 10, 2, 7, 3, 8, 4, 9], [72] * 10, ref=14, bankIntervals=[[144] * 5, [144] * 5])
add("ref-15-v12-60", "V12, 60 degrees", "1-7-5-11-3-9-6-12-2-8-4-10, even 60; cylinders 1-6 one bank.",
    vee([0] * 6 + [1] * 6, [330, 210, 90, 90, 210, 330] * 2, [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1], [-30, 30],
        [0, 1, 2, 3, 4, 5] * 2),
    [1, 7, 5, 11, 3, 9, 6, 12, 2, 8, 4, 10], [60] * 12, ref=15, bankIntervals=[[120] * 6, [120] * 6])
add("ref-16-rotary-2", "Rotary, two rotors", "Rotors 180 degrees apart on the eccentric shaft: a face fires each 180.",
    dict(bank=[0, 0], pin=[0, 180], rev=[0, 0], place=[0.0, 0.1], bankAngle=[0]),
    [1, 4, 2, 5, 3, 6], [180] * 6, ref=16, rotors=2)

# ---- the others ----------------------------------------------------------------------------------------------------
add("v4-90", "V4, 90 degrees, 180-degree crank", "Two pins 180 apart, each shared by a front and a rear cylinder "
    "(a sport-touring V4's): 180-90-180-270.",
    vee([0, 1, 0, 1], [45, 45, 225, 225], [0, 0, 0, 1], [45, -45], [0, 0, 1, 1]),
    [1, 3, 2, 4], [180, 90, 180, 270])
add("v4-65", "V4, 65 degrees, 180-degree crank", "Two pins 180 apart, each shared by a front and a rear cylinder.",
    vee([0, 1, 0, 1], [32.5, 32.5, 212.5, 212.5], [0, 0, 0, 1], [32.5, -32.5], [0, 0, 1, 1]),
    [1, 3, 2, 4], [180, 115, 180, 245])
add("vr6", "VR6, 15 degrees", "One head, two staggered rows 15 degrees apart; six pins offset so it fires evenly, "
    "1-5-3-6-2-4.",
    dict(bank=[0, 1, 0, 1, 0, 1], pin=[7.5, 232.5, 127.5, 112.5, 247.5, 352.5], rev=[0, 1, 0, 1, 0, 1],
         place=[0.0, 0.05, 0.10, 0.15, 0.20, 0.25], bankAngle=[7.5, -7.5]),
    [1, 5, 3, 6, 2, 4], [120] * 6)
add("v6-90-common", "V6, 90 degrees, common pins", "Three pins 120 apart, each shared: odd-firing 90-150, as the "
    "first 90-degree V6s cut from V8s.",
    vee([0, 1, 0, 1, 0, 1], [45, 45, 285, 285, 165, 165], [0, 1, 1, 1, 0, 0], [45, -45], [0, 0, 1, 1, 2, 2]),
    [1, 6, 5, 4, 3, 2], [150, 90, 150, 90, 150, 90])
add("v6-90-split", "V6, 90 degrees, 30-degree split pins", "Each shared pin split by 30 degrees: even 120, "
    "1-6-5-4-3-2.",
    vee([0, 1, 0, 1, 0, 1], [45, 75, 285, 315, 165, 195], [0, 1, 1, 1, 0, 0], [45, -45], [0, 0, 1, 1, 2, 2]),
    [1, 6, 5, 4, 3, 2], [120] * 6)
add("inline-8", "Straight eight", "1-6-2-5-8-3-7-4, even 90 (a split four-and-four crank).",
    inline([0, 180, 270, 90, 90, 270, 180, 0], [0, 0, 1, 1, 0, 0, 1, 1]), [1, 6, 2, 5, 8, 3, 7, 4], [90] * 8)
add("v10-90", "V10, 90 degrees, 18-degree split pins", "1-6-5-10-2-7-3-8-4-9, even 72: each pin split by 18 degrees.",
    vee([0] * 5 + [1] * 5, [315, 27, 243, 99, 171, 333, 45, 261, 117, 189], [0, 0, 1, 1, 0, 0, 1, 1, 1, 0],
        [-45, 45], [0, 1, 2, 3, 4, 0, 1, 2, 3, 4]),
    [1, 6, 5, 10, 2, 7, 3, 8, 4, 9], [72] * 10)
add("v16-45", "V16, 45 degrees", "Two straight eights on one eight-throw crank, 45 degrees apart: even 45.",
    vee([0] * 8 + [1] * 8, [337.5, 157.5, 247.5, 67.5, 67.5, 247.5, 157.5, 337.5] * 2,
        [0, 0, 1, 1, 0, 0, 1, 1] * 2, [-22.5, 22.5], list(range(8)) * 2),
    [1, 9, 6, 14, 2, 10, 5, 13, 8, 16, 3, 11, 7, 15, 4, 12], [45] * 16)
add("triple-tplane", "Inline three, T-plane crank", "The outer pins opposed, the middle at 90 degrees: 1-3-2, "
    "180-270-270.",
    inline([0, 270, 180], [0, 1, 0]), [1, 3, 2], [180, 270, 270])
add("twin-180", "Parallel twin, 180-degree crank", "Pins opposed: 180-540.",
    inline([0, 180], [0, 0]), [1, 2], [180, 540])
add("vtwin-60", "V-twin, 60 degrees, one pin", "The rear cylinder 60 degrees behind the front: 300-420.",
    vee([0, 1], [30, 30], [0, 0], [30, -30], [0, 0]), [1, 2], [300, 420])
add("vtwin-90", "V-twin, 90 degrees, one pin", "An L-twin: 270-450.",
    vee([0, 1], [45, 45], [0, 0], [45, -45], [0, 0]), [1, 2], [270, 450])
add("rotary-3", "Rotary, three rotors", "Rotors 120 degrees apart: a face fires each 120 degrees of the shaft.",
    dict(bank=[0, 0, 0], pin=[0, 240, 120], rev=[0, 0, 0], place=[0.0, 0.1, 0.2], bankAngle=[0]),
    [1, 4, 7, 2, 5, 8, 3, 6, 9], [120] * 9, rotors=3)
add("rotary-4", "Rotary, four rotors", "Rotors 90 degrees apart (a racing four-rotor's): a face fires each 90.",
    dict(bank=[0, 0, 0, 0], pin=[0, 180, 270, 90], rev=[0, 0, 0, 0], place=[0.0, 0.1, 0.2, 0.3], bankAngle=[0]),
    [1, 7, 4, 10, 2, 8, 5, 11, 3, 9, 6, 12], [90] * 12, rotors=4)

# W12 and W16: four banks (two narrow pairs). Pins derived here from the firing order the engines are published with,
# and written as geometry: the even intervals are what the narrow-angle offsets are for.
def w_engine(name, title, notes, banks, bankAngles, order, cyc_iv):
    n = len(banks)
    fire = [0.0] * n
    for k, cyl in enumerate(order):
        fire[cyl - 1] = k * cyc_iv
    pins, revs = [], []
    for c in range(n):
        tdc = fire[c] % 360.0
        pins.append(round((bankAngles[banks[c]] - tdc) % 360.0, 3))
        revs.append(1 if fire[c] >= 360.0 else 0)
    place = [round(PITCH * 0.5 * c, 3) for c in range(n)]
    add(name, title, notes, dict(bank=banks, pin=pins, rev=revs, place=place, bankAngle=bankAngles), order,
        [cyc_iv] * n)

w_engine("w12", "W12, two VR6s at 72 degrees", "Banks at -43.5, -28.5, 28.5 and 43.5 degrees; 1-12-5-8-3-10-6-7-2-11-4-9.",
         [0, 1, 0, 1, 0, 1, 2, 3, 2, 3, 2, 3], [-43.5, -28.5, 28.5, 43.5], [1, 12, 5, 8, 3, 10, 6, 7, 2, 11, 4, 9], 60)
w_engine("w16", "W16, two VR8s at 90 degrees", "Banks at -52.5, -37.5, 37.5 and 52.5 degrees; "
         "1-14-9-4-7-16-11-6-15-10-3-8-13-2-5-12.",
         [0, 1, 0, 1, 0, 1, 0, 1, 2, 3, 2, 3, 2, 3, 2, 3], [-52.5, -37.5, 37.5, 52.5],
         [1, 14, 9, 4, 7, 16, 11, 6, 15, 10, 3, 8, 13, 2, 5, 12], 45)

def bank_intervals(e, fire, cyc):
    out = []
    for b in range(len(e["bankAngle"])):
        cs = sorted([fire[c] for c in range(len(fire)) if e["bank"][c] == b])
        out.append([round((cs[(j + 1) % len(cs)] - cs[j]) % cyc or cyc, 4) for j in range(len(cs))])
    return out

def same_cyclic(a, b):
    return len(a) == len(b) and any(a[k:] + a[:k] == b for k in range(len(a)))

REF_FA = json.load(open(os.path.join(os.path.dirname(__file__), "ref_firing.json")))

bad = 0
os.makedirs(OUT, exist_ok=True)
for e in E:
    fire, cyc = derive(e)
    order, iv = order_intervals(fire, cyc)
    ok = order == e["order"] and iv == [float(x) for x in e["intervals"]]
    if e["ref"] is not None:
        ok = ok and fire == [float(x) for x in REF_FA[e["ref"]][:len(fire)]]
    if e["bankIntervals"] and not e.get("rotors"):
        bi = bank_intervals(e, fire, cyc)
        ok = ok and all(same_cyclic(x, [float(v) for v in y]) for x, y in zip(bi, e["bankIntervals"]))
    print(("ok  " if ok else "BAD ") + e["name"], order, iv)
    if not ok:
        print("     expected", e["order"], e["intervals"], "fire", fire)
        bad += 1
        continue
    engine = {"custom": True}
    if e["ref"] is not None:
        engine["layout"] = e["ref"]
    if e["rotors"]:
        engine["rotors"] = e["rotors"]
    engine["cylinders"] = len(e["pin"])
    engine["bankAngle"] = e["bankAngle"]
    engine["bank"] = e["bank"]
    engine["pin"] = e["pin"]
    engine["fireRev"] = e["rev"]
    engine["place"] = e["place"]
    doc = {"format": "tee-build", "version": 1, "name": e["title"], "notes": e["notes"],
           "spec": {"engine": engine},
           "expect": {"firingOrder": e["order"], "firingIntervals": e["intervals"], "firingAngles": fire}}
    with open(os.path.join(OUT, e["name"] + ".json"), "w") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")
sys.exit(1 if bad else 0)
