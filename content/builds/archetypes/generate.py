#!/usr/bin/env python3
"""Writes the archetype builds of content/builds/archetypes/ (card P) from the table below.

Run from the repository's root: python3 content/builds/archetypes/generate.py
Each archetype is a whole build (engine, car, tune) with every model switch on (the custom engine, the valve lab,
the induction model, the boost machine, the exhaust builder, the fuels, the ECU's maps, the vehicle), air at a
standard day (288.15 K, 101325 Pa). The engines' crank geometries are the layout presets of content/builds/engines/
(card D's), copied in so that each build stands alone. Dimensions: the sources named in each entry's notes and in
content/validation/archetypes/<slug>.json. The parts alone (engines, cars, tunes) are written to ../parts/.
"""
import json
import math
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
ENGINES = os.path.join(ROOT, "content", "builds", "engines")
PARTS = os.path.join(ROOT, "content", "builds", "parts")

RPM = math.pi / 30.0          # rpm to rad/s
BAR = 100000.0


def geometry(preset):
    with open(os.path.join(ENGINES, preset + ".json")) as f:
        e = json.load(f)["spec"]["engine"]
    return {k: e[k] for k in ("cylinders", "bankAngle", "bank", "pin", "fireRev", "place", "rotors") if k in e}


# ---- the cars, by class ----------------------------------------------------------------------------------------
# mass kg (with a driver), drive, wheelbase m, cg height m, front weight share, track m, drag area m^2, tyre,
# rolling radius m, gearbox, ratios, final drive. Classes' figures: typical published kerb weights, drag areas
# (Cd x A) and gearing of production cars of each kind (from memory; firm to some 10 %). launch: the car has launch
# control (every dual clutch and the rally car's sequential: their makers fit it), as ecu.launch.on at its default speed.
# The front-mid GT's rear differential is limited-slip, as the class's cars have it (an electronic one; here a 100 N m
# preload and a torque bias of 0.5: judgement, the lead's question of 19:36).
CARS = {
    "sports-sedan": dict(mass=1700, drive="Rear", wheelbase=2.85, cgHeight=0.52, frontWeight=0.52, track=1.58, cdA=0.66,
                         tyre="Sport", tyreRadius=0.335, gearbox="Dual clutch", launch=True,
                         ratio=[4.81, 2.59, 1.70, 1.28, 1.00, 0.84, 0.67], finalDrive=3.15, wheelInertia=1.4),
    "mid-engine": dict(mass=1550, drive="Rear", wheelbase=2.65, cgHeight=0.45, frontWeight=0.42, track=1.65, cdA=0.62,
                       tyre="Semi-slick", tyreRadius=0.345, gearbox="Dual clutch", launch=True,
                       ratio=[3.08, 2.19, 1.63, 1.29, 1.03, 0.84, 0.69], finalDrive=4.44, wheelInertia=1.5),
    "rear-engine-classic": dict(mass=1450, drive="Rear", wheelbase=2.27, cgHeight=0.48, frontWeight=0.40, track=1.40,
                                cdA=0.62, tyre="Street", tyreRadius=0.31, gearbox="Manual",
                                ratio=[3.50, 2.12, 1.43, 1.03, 0.79], finalDrive=3.44, wheelInertia=1.1),
    "muscle-auto": dict(mass=1800, drive="Rear", wheelbase=2.84, cgHeight=0.55, frontWeight=0.57, track=1.52, cdA=0.88,
                        tyre="Street", tyreRadius=0.345, gearbox="Automatic", ratio=[2.48, 1.48, 1.00],
                        finalDrive=3.73, liveAxle=True, wheelInertia=1.6),
    "muscle-manual": dict(mass=1600, drive="Rear", wheelbase=2.74, cgHeight=0.54, frontWeight=0.56, track=1.52, cdA=0.82,
                          tyre="Street", tyreRadius=0.34, gearbox="Manual", ratio=[2.52, 1.88, 1.46, 1.00],
                          finalDrive=3.55, liveAxle=True, wheelInertia=1.5),
    "light-coupe-rotary": dict(mass=1330, drive="Rear", wheelbase=2.43, cgHeight=0.46, frontWeight=0.50, track=1.46,
                               cdA=0.55, tyre="Sport", tyreRadius=0.315, gearbox="Manual",
                               ratio=[3.48, 2.02, 1.39, 1.00, 0.72], finalDrive=4.10, wheelInertia=1.1),
    "hot-hatch-light": dict(mass=1100, drive="Front", wheelbase=2.47, cgHeight=0.52, frontWeight=0.62, track=1.43,
                            cdA=0.62, tyre="Sport", tyreRadius=0.29, gearbox="Manual",
                            ratio=[3.23, 1.90, 1.28, 0.97, 0.76], finalDrive=4.27, wheelInertia=0.9),
    "super-coupe": dict(mass=1950, drive="Rear", wheelbase=2.85, cgHeight=0.52, frontWeight=0.54, track=1.60, cdA=0.75,
                        tyre="Sport", tyreRadius=0.35, gearbox="Manual", ratio=[2.66, 1.78, 1.30, 1.00, 0.74, 0.50],
                        finalDrive=3.73, wheelInertia=1.6),
    "awd-supercar": dict(mass=1650, drive="All", wheelbase=2.65, cgHeight=0.46, frontWeight=0.42, track=1.64, cdA=0.65,
                         tyre="Semi-slick", tyreRadius=0.345, gearbox="Dual clutch", launch=True,
                         ratio=[3.13, 2.59, 1.96, 1.55, 1.24, 1.00, 0.85], finalDrive=4.0, centreSplit=0.3,
                         wheelInertia=1.5),
    "front-mid-gt": dict(mass=1750, drive="Rear", wheelbase=2.72, cgHeight=0.47, frontWeight=0.47, track=1.66, cdA=0.68,
                         tyre="Semi-slick", tyreRadius=0.35, gearbox="Dual clutch", launch=True,
                         ratio=[3.08, 2.19, 1.63, 1.29, 1.03, 0.84, 0.69], finalDrive=4.8, wheelInertia=1.6,
                         rearPreload=100.0, rearKTorque=0.5),
    "rally-awd": dict(mass=1300, drive="All", wheelbase=2.52, cgHeight=0.50, frontWeight=0.58, track=1.50, cdA=0.75,
                      tyre="Semi-slick", tyreRadius=0.32, gearbox="Sequential", launch=True,
                      ratio=[2.92, 2.12, 1.67, 1.36, 1.13, 0.96], finalDrive=4.11, centreSplit=0.5, wheelInertia=1.2),
    "awd-sedan-turbo": dict(mass=1550, drive="All", wheelbase=2.63, cgHeight=0.52, frontWeight=0.58, track=1.53,
                            cdA=0.72, tyre="Sport", tyreRadius=0.325, gearbox="Manual",
                            ratio=[3.64, 2.24, 1.52, 1.13, 0.92, 0.75], finalDrive=3.90, centreSplit=0.41,
                            wheelInertia=1.3),
    "roadster": dict(mass=1250, drive="Rear", wheelbase=2.40, cgHeight=0.46, frontWeight=0.50, track=1.47, cdA=0.62,
                     tyre="Sport", tyreRadius=0.32, gearbox="Manual", ratio=[3.13, 2.05, 1.48, 1.16, 0.97, 0.81],
                     finalDrive=4.10, wheelInertia=1.1),
    "kei": dict(mass=780, drive="Rear", wheelbase=2.23, cgHeight=0.45, frontWeight=0.45, track=1.30, cdA=0.55,
                tyre="Sport", tyreRadius=0.275, gearbox="Manual", ratio=[3.31, 2.06, 1.46, 1.09, 0.85],
                finalDrive=4.55, wheelInertia=0.7),
    "family-diesel": dict(mass=1500, drive="Front", wheelbase=2.64, cgHeight=0.55, frontWeight=0.62, track=1.55,
                          cdA=0.66, tyre="Economy", tyreRadius=0.315, gearbox="Manual",
                          ratio=[3.77, 2.09, 1.32, 0.98, 0.76, 0.64], finalDrive=3.39, wheelInertia=1.1),
    "tractor": dict(kind="Truck", truckAir=True, mass=36000, drive="Rear", wheelbase=5.8, cgHeight=1.5, frontWeight=0.25, track=2.0,
                    cdA=6.0, tyre="Truck", tyreRadius=0.51, gearbox="Eighteen-speed",
                    ratio=[14.4, 12.29, 8.51, 7.26, 6.05, 5.16, 4.38, 3.74, 3.20, 2.73, 2.29, 1.95, 1.62, 1.38, 1.17,
                           1.00, 0.86, 0.73], finalDrive=3.55, wheelInertia=40.0),
    "small-hatch": dict(mass=1200, drive="Front", wheelbase=2.56, cgHeight=0.54, frontWeight=0.62, track=1.52, cdA=0.62,
                        tyre="Street", tyreRadius=0.31, gearbox="Manual", ratio=[3.69, 1.96, 1.30, 1.03, 0.85, 0.72],
                        finalDrive=4.06, wheelInertia=0.9),
    "compact-cvt": dict(mass=1250, drive="Front", wheelbase=2.60, cgHeight=0.54, frontWeight=0.62, track=1.52, cdA=0.64,
                        tyre="Economy", tyreRadius=0.31, gearbox="CVT", ratio=[2.5], finalDrive=4.4, wheelInertia=0.9),
    "hot-hatch-dct": dict(mass=1450, drive="Front", wheelbase=2.63, cgHeight=0.53, frontWeight=0.62, track=1.54,
                          cdA=0.68, tyre="Sport", tyreRadius=0.32, gearbox="Dual clutch", launch=True,
                          ratio=[3.56, 2.53, 1.68, 1.02, 0.79, 0.64], finalDrive=3.65, wheelInertia=1.1),
    "family-sedan-auto": dict(mass=1650, drive="Front", wheelbase=2.78, cgHeight=0.55, frontWeight=0.62, track=1.58,
                              cdA=0.68, tyre="Street", tyreRadius=0.33, gearbox="Automatic",
                              ratio=[3.30, 1.90, 1.42, 1.00, 0.71, 0.61], finalDrive=3.30, wheelInertia=1.2),
    "pickup-4wd": dict(mass=2500, drive="All", wheelbase=3.68, cgHeight=0.75, frontWeight=0.57, track=1.71, cdA=1.25,
                       tyre="Street", tyreRadius=0.39, gearbox="Automatic",
                       ratio=[4.70, 2.99, 2.15, 1.80, 1.52, 1.28, 1.00, 0.85, 0.69, 0.64], finalDrive=3.55,
                       centreSplit=0.4, liveAxle=True, wheelInertia=2.2),
    "heavy-pickup": dict(mass=3400, drive="Rear", wheelbase=3.79, cgHeight=0.8, frontWeight=0.60, track=1.73, cdA=1.35,
                         tyre="Truck", tyreRadius=0.4, gearbox="Automatic", ratio=[3.23, 2.19, 1.41, 1.00, 0.82, 0.62],
                         finalDrive=3.42, liveAxle=True, wheelInertia=3.0),
    "awd-autobahn": dict(mass=2050, drive="All", wheelbase=2.92, cgHeight=0.52, frontWeight=0.55, track=1.63, cdA=0.70,
                         tyre="Sport", tyreRadius=0.345, gearbox="Automatic",
                         ratio=[5.00, 3.20, 2.14, 1.72, 1.31, 1.00, 0.82, 0.64], finalDrive=3.15, centreSplit=0.4,
                         wheelInertia=1.5),
    "hatch-vr": dict(mass=1300, drive="Front", wheelbase=2.47, cgHeight=0.53, frontWeight=0.63, track=1.47, cdA=0.66,
                     tyre="Street", tyreRadius=0.30, gearbox="Manual", ratio=[3.78, 2.12, 1.43, 1.03, 0.84],
                     finalDrive=3.39, wheelInertia=1.0),
    "coupe-six": dict(mass=1550, drive="Rear", wheelbase=2.73, cgHeight=0.50, frontWeight=0.51, track=1.52, cdA=0.63,
                      tyre="Sport", tyreRadius=0.335, gearbox="Manual", ratio=[4.23, 2.53, 1.67, 1.23, 1.00, 0.83],
                      finalDrive=3.62, wheelInertia=1.3),
    "pony-car": dict(mass=1750, drive="Rear", wheelbase=2.72, cgHeight=0.53, frontWeight=0.54, track=1.58, cdA=0.75,
                     tyre="Sport", tyreRadius=0.345, gearbox="Manual", ratio=[3.66, 2.43, 1.69, 1.32, 1.00, 0.65],
                     finalDrive=3.73, wheelInertia=1.5),
    "front-engine-sports": dict(mass=1500, drive="Rear", wheelbase=2.69, cgHeight=0.47, frontWeight=0.51, track=1.60,
                                cdA=0.62, tyre="Semi-slick", tyreRadius=0.34, gearbox="Manual",
                                ratio=[2.66, 1.78, 1.30, 1.00, 0.74, 0.50], finalDrive=3.42, wheelInertia=1.4),
    "big-sedan-fwd": dict(mass=1650, drive="Front", wheelbase=2.85, cgHeight=0.56, frontWeight=0.63, track=1.56,
                          cdA=0.72, tyre="Economy", tyreRadius=0.33, gearbox="Automatic", ratio=[2.92, 1.57, 1.00, 0.70],
                          finalDrive=3.05, wheelInertia=1.2),
}


def car_section(name):
    c = dict(CARS[name])
    c.pop("launch", None)
    out = {"model": "Vehicle", "kind": c.pop("kind", "Car")}
    ratios = c.pop("ratio")
    out.update(c)
    out["gears"] = len(ratios) if out["gearbox"] != "CVT" else 1
    if out["gearbox"] == "CVT":
        out["cvtLow"], out["cvtHigh"] = 2.6, 0.42
    out["autoGearing"] = False
    out["ratio"] = ratios
    out.setdefault("brakeDecel", 1.1 if out["kind"] == "Car" else 0.5)
    out["abs"] = True
    return out


# ---- the archetypes ----------------------------------------------------------------------------------------------
# Engine: geometry preset, bore and stroke (mm), rod (mm), compression, valves a cylinder and their diameters (mm),
# the cams (seat and 1 mm durations, lift mm, centreline, crank degrees), the train, the redline and idle (rpm).
# The redline is the limiter's speed. Notes say where each figure is from.
A = []


def arch(**k):
    A.append(k)


arch(slug="straight-six-twin-turbo-30", runner=0.3, name="3.0 L straight six, twin turbo", geo="ref-10-inline-6",
     layout="Inline 6", bore=84.0, stroke=89.6, rod=144.35, cr=10.2, valves=4, dIn=31.4, dEx=28.0,
     cam=(252, 220, 9.7, 105, 252, 220, 9.7, 105), train="Finger", phase=(70, 55), redline=7000, idle=650,
     boost=dict(kind="Twin turbos", layout="Two, a bank each", target=1.0, spring=0.7, scrolls="Twin scroll"),
     exhaust={"primaryTo": [0, 0, 0, 1, 1, 1], "sides": 2, "collectorTo": [0, 1]},
     direct=True, grade="Premium (91)", car="sports-sedan", timing="Chain",
     notes="A modern 3 L twin-turbo straight six with direct injection: 84 x 89.6 mm (2979 cc), 10.2:1, four valves, "
           "both cams phased, two twin-scroll turbos each fed by three cylinders at about 1 bar (cylinders 1-3 and 4-6 each into its own collector and turbo, a dual exhaust). Production engines "
           "of this kind make 250 to 340 kW and 450 to 600 N m (makers' figures). In a rear-drive sports sedan with "
           "a seven-speed dual clutch.")
arch(slug="flat-plane-v8-45", name="Flat-plane 4.5 L V8", geo="ref-13-v8-flat", layout="V8 flat-plane",
     bore=94.0, stroke=81.0, rod=150.0, cr=12.5, valves=4, dIn=36.5, dEx=30.5,
     cam=(276, 248, 12.0, 106, 268, 240, 11.0, 110), train="Finger", phase=(50, 40), redline=9000, idle=1000,
     intake=dict(runnerLength=0.26, runnerLength2=0.18, runnerSwitch=6200 * RPM), direct=True, grade="Premium (91)",
     car="mid-engine", timing="Chain", lifters="Solid",
     notes="A high-revving flat-plane V8: 94 x 81 mm (4497 cc), 12.5:1, direct injection, four valves, 9000 rpm "
           "(the published figures of the 4.5 L 90-degree flat-plane V8 of 2009, F136-type: 419 kW at 9000 rpm, "
           "540 N m at 6000; Wikipedia and the maker's site). Variable-length runners. In a mid-engined coupe.")
arch(slug="air-cooled-flat-six-36", runner=0.35, name="Air-cooled flat six", geo="ref-11-flat-6", layout="Flat 6",
     bore=100.0, stroke=76.4, rod=127.0, cr=11.3, valves=2, dIn=49.0, dEx=42.5,
     cam=(256, 222, 11.5, 108, 252, 216, 10.5, 112), train="Rocker", redline=6700, idle=850,
     grade="Premium (91)", car="rear-engine-classic", timing="Chain", fan="Viscous clutch",
     notes="A 3.6 L air- and oil-cooled flat six, two valves a cylinder on rockers, dry sump: 100 x 76.4 mm "
           "(3600 cc), 11.3:1; 184 kW at 6100 rpm and 310 N m at 4800 (the 1989 M64-type engine; Wikipedia, "
           "auto-data.net). The real intake is a two-stage resonance intake, a "
           "flap between its two plenum halves (closed some 3000 to 5000-5500 rpm at large throttle: forums only, "
           "Rennlist, Pelican Parts), its runners' lengths not found in any source. The cooling fan's whir is the "
           "viscous fan. In a rear-engined coupe, five-speed manual.")
arch(slug="big-block-74-pushrod-v8", name="Big-block 7.4 L pushrod V8", geo="ref-12-v8-cross",
     # the 1972 LS5's 8.5:1 (over-drive-magazine.com's 1972 Chevelle fact sheet; a corvetteforum post agrees; one
     # other says 8.25), the net-rated engine its band is (the lead, 20:46: hardware and target the same engine)
     # its cam and valves: the LS5's cam 3883986, 213/217 deg (at 0.050 in, as quoted elsewhere) and .460/.488 in
     # lift, the oval-port heads' 2.06/1.72 in valves (clacoa.com's big-block ID table, an enthusiast reference); the
     # seat 268/274 deg (a corvetteforum post, unverified)
     layout="V8 crossplane", bore=107.95, stroke=101.6, rod=155.8, cr=8.5, valves=2, dIn=52.3, dEx=43.7,
     cam=(268, 213, 11.7, 110, 274, 217, 12.4, 114), train="Pushrod", redline=5600, idle=650,
     intake=dict(runnerLength=0.20, plenumVolume=0.003), friction="Flat tappet", grade="Regular (87)",
     car="muscle-auto", timing="Chain", fan="Viscous clutch",
     notes="A 1972 454 cubic inch big block (the LS5): 4.25 x 4.00 in (107.95 x 101.6 mm, 7440 cc), 8.5:1, two "
           "valves on pushrods and the LS5's flat-tappet hydraulic cam (3883986: clacoa.com), one four-barrel-sized "
           "throttle over a single plenum. Rated 270 hp SAE net at 4000 rpm and 390 lb ft at 3200 (201 kW, 529 N m: "
           "carnut.com, over-drive-magazine.com); 1973-74's LS4 270-275 hp. In a heavy muscle car with a three-speed "
           "automatic.")
arch(slug="rotary-twin-13", name="Rotary twin", geo="ref-16-rotary-2", layout="Rotary", displacement=0.001308,
     cr=9.0, redline=8000, idle=800,
     boost=dict(kind="Twin turbos", layout="Sequential", target=0.7, spring=0.5),
     grade="Premium (91)", car="light-coupe-rotary", timing="Gears",
     notes="A twin-rotor Wankel, 2 x 654 cc, 9.0:1, with two sequential turbos (the 13B-REW kind: 206 to 280 PS, "
           "the latter at 6500 rpm, some 294 N m at 5000; Wikipedia, makers' figures, firm to 10 %). In a light "
           "rear-drive coupe.")
arch(slug="rotary-twin-na-side-port", clutchTorque=350.0, rotaryCam=0.15, rotaryPorts=(146.7, 362.0, 362.0, 583.3), name="Rotary twin, side ports", geo="ref-16-rotary-2", layout="Rotary",
     displacement=0.001308, cr=10.0, redline=9000, idle=800,
     intake=dict(runnerLength=0.45, runnerLength2=0.30, runnerSwitch=5500 * RPM),
     exhaust={"converter": [2, 2], "mufKind": [1, 1]}, grade="Premium (91)", car="light-coupe-rotary", timing="Gears",
     notes="A naturally aspirated twin-rotor Wankel with side exhaust ports, 2 x 654 cc, 10:1, a 9000 rpm limiter and a "
           "two-stage intake; its side ports' timing EO 50 BBDC, EC 3 ATDC, IO 3 ATDC, IC 65 ABDC in eccentric-shaft degrees (N1, from memory, to some 10 degrees) (the RENESIS kind of 2003: 170 to 184 kW at 8200 to 8500 rpm, 211 to 216 N m at 5500; "
           "makers' figures by market and year, from memory, firm to 5 %). High-flow converter, straight-through "
           "mufflers. In a light rear-drive coupe.")
arch(slug="hot-hatch-16-16v", runner=0.35, convK=18.0, name="1.6 L 16-valve hot hatch", geo="ref-05-inline-4", layout="Inline 4",
     bore=81.0, stroke=77.4, rod=134.0, cr=10.4, valves=4, dIn=31.0, dEx=27.0,
     cam=(256, 224, 9.6, 108, 248, 216, 8.8, 110), train="Bucket", redline=7200, idle=850,
     car="hot-hatch-light", timing="Toothed belt",
     notes="A light front-drive hatch with a twin-cam 16-valve 1.6: 81 x 77.4 mm (1595 cc), 10.4:1; engines of the "
           "kind made 85 to 118 kW at 6500 to 7600 rpm and 140 to 160 N m (makers' figures of the 1980s and 90s).")
arch(slug="supercharged-v8-62", runner=0.3, name="Supercharged 6.2 L V8", geo="ref-12-v8-cross", layout="V8 crossplane",
     bore=103.25, stroke=92.0, rod=154.0, cr=9.1, valves=2, dIn=54.0, dEx=40.4,
     cam=(270, 213, 12.6, 113, 270, 220, 12.4, 118), train="Pushrod", redline=6200, idle=700,
     boost=dict(kind="Supercharger", charger="Twin-screw", target=0.62, scDisplacement=0.0019),
     friction="Roller", grade="Premium (91)", car="super-coupe", timing="Chain", blowerDrive="Belt",
     limiter="Soft",
     notes="The 6.2 L pushrod V8 of 103.25 x 92 mm (6162 cc) at 9.1:1 under a 1.9 L positive-displacement blower at "
           "some 0.6 bar (the LSA kind: 415 kW at 6100 rpm, 746 N m at 3800; gmauthority.com, a community spec "
           "table for the compression). In a heavy six-speed coupe.")
arch(slug="small-block-57-carb", name="Small-block 5.7 L, carburettor-like", geo="ref-12-v8-cross",
     # the L48's 8.5:1 (corvsport.com's 1977 table; the L82 9.0) and its "2 into 1 into 2" exhaust: both banks into
     # one converter, split again behind it to two mufflers (restore-an-old-car.com's C3 exhaust; NCRS forum: the last
     # true duals 1974): here one side through one converter and muffler (N3, 20:34)
     layout="V8 crossplane", bore=101.6, stroke=88.4, rod=144.8, cr=8.5, valves=2, dIn=49.3, dEx=38.1,
     exhaust={"sides": 1, "converter": [1, 1], "mufKind": [2, 2]},
     # the L48's cam (N3, 20:06: toward the target's middle, not the hotter L82's): about 195/202 deg at 0.050 in and
     # 0.390/0.410 in lift (from memory, firm to some 5 deg), the seat some 54 deg longer as this hydraulic flat tappet's
     cam=(251, 197, 9.9, 110, 256, 204, 10.4, 114), train="Pushrod", redline=5500, idle=700,
     intake=dict(runnerLength=0.18, plenumVolume=0.0025), friction="Flat tappet", grade="Regular (87)",
     car="muscle-manual", timing="Chain", fan="Viscous clutch",
     notes="A 350 cubic inch small block (4.00 x 3.48 in: 101.6 x 88.4 mm, 5733 cc), 8.5:1, two valves on pushrods, a "
           "flat-tappet cam (the L48's), one big throttle over an open plenum as a four-barrel carburettor's. The L48 "
           "of 1975 to 1980 is rated 123 to 145 kW net (165 to 195 hp) and 346 to 380 N m (255 to 280 lb ft), 1977's "
           "180 hp at 4000 rpm and 270 lb ft at 2400; the L82 157 kW at 5200 and 346 N m at 3600 (SAE net, looked up: "
           "corvsport.com, corvetteactioncenter.com). One converter for both banks (2 into 1 into 2). Four-speed manual, "
           "live axle.")
arch(slug="v10-52", runner=0.28, name="V10 5.2 L", geo="v10-90", layout="V10", bore=84.5, stroke=92.8, rod=154.0, cr=12.5,
     valves=4, dIn=33.8, dEx=28.0, cam=(268, 240, 11.8, 108, 260, 232, 10.8, 112), train="Finger", phase=(42, 42),
     redline=8700, idle=900, direct=True, grade="Premium (91)", car="awd-supercar", timing="Chain",
     notes="A 90-degree V10 with split crankpins for even firing: 84.5 x 92.8 mm (5204 cc), 12.5:1, direct and port "
           "injection, 8700 rpm (the 5.2 L V10 of 2008 on: 386 to 470 kW at 8000 to 8700 rpm, 530 to 565 N m at "
           "6500; makers' figures). In an all-wheel-drive mid-engined car.")
arch(slug="v12-65", runner=0.28, name="V12 6.5 L", geo="ref-15-v12-60", layout="V12", bore=95.0, stroke=76.4, rod=145.0,
     cr=13.5, valves=4, dIn=37.0, dEx=31.0, cam=(272, 246, 12.2, 106, 264, 238, 11.2, 110), train="Finger",
     phase=(50, 40), redline=8900, idle=900, direct=True, grade="Premium (91)", car="front-mid-gt", timing="Chain",
     lifters="Solid",
     notes="A naturally aspirated 6.5 L V12 of the 2010s: 95 x 76.4 mm (6496 cc), 13.5:1, direct injection, four "
           "valves, all cams phased (engines of the kind: 515 to 588 kW at 8250 to 9000 rpm, 690 to 720 N m at "
           "6000 to 7000; makers' figures). In a front-mid-engined GT with a seven-speed dual clutch.")
arch(slug="inline-five-turbo-rally", runner=0.3, name="Inline five turbo rally", geo="ref-08-inline-5", layout="Inline 5",
     bore=81.0, stroke=86.4, rod=144.0, cr=9.3, valves=4, dIn=30.0, dEx=26.5,
     cam=(252, 220, 10.0, 108, 248, 216, 9.5, 110), train="Bucket", redline=7000, idle=1000,
     boost=dict(kind="Turbo", target=1.3, spring=0.8, antiLag=True, blowOff="Recirculating"),
     grade="Race", car="rally-awd", timing="Toothed belt", lifters="Solid", antiLag=True, limiter="Spark cut",
     notes="A 2.2 L 20-valve straight five, 81 x 86.4 mm (2226 cc), 9.3:1 (the 3B kind: 162 kW at 5700 rpm, 309 N m "
           "road trim; audiworld.com), here in rally trim: 1.3 bar, anti-lag, a spark-cut limiter, race fuel; "
           "rally engines of the kind made some 220 to 260 kW. Sequential gearbox, all-wheel drive.")
arch(slug="boxer-four-turbo-unequal", runner=0.3, name="Boxer four turbo with unequal headers", geo="ref-06-flat-4",
     layout="Flat 4", bore=99.5, stroke=79.0, rod=130.5, cr=8.2, valves=4, dIn=36.0, dEx=32.0,
     cam=(256, 228, 10.0, 110, 252, 224, 9.6, 112), train="Bucket", phase=(35, 0), redline=7000, idle=800,
     boost=dict(kind="Turbo", target=1.0, spring=0.7), grade="Premium (91)", car="awd-sedan-turbo",
     timing="Toothed belt", exhaust=dict(primSpread=0.25),
     notes="A 2.5 L boxer four with one turbo, 99.5 x 79 mm (2457 cc), 8.2:1 (the EJ257 kind: 221 kW at 6000 rpm, "
           "393 N m at 4000; Wikipedia and enthusiasts' tables). Its headers' primaries of unequal length are the "
           "burble: written in the exhaust builder (primSpread), which the program does not yet run. All-wheel "
           "drive, six-speed manual.")
arch(slug="two-stage-cam-20", runner=0.3, convK=16.0, name="Two-stage-cam 2.0 L", geo="ref-05-inline-4", layout="Inline 4",
     bore=87.0, stroke=84.0, rod=153.0, cr=11.7, valves=4, dIn=34.0, dEx=29.0,
     cam=(236, 204, 8.5, 110, 232, 200, 8.0, 112), train="Rocker", redline=9000, idle=900,
     # the high stage as the K20A Type R's (N3, 20:18): 280/278 deg, 12.65/12.14 mm lift (motorreviewer.com, an
     # aggregator that does not say where duration is measured: read as the seat; 1 mm some 32 deg shorter, as the low
     # stage's), switched at 6000 rpm (the DC5's: honda.co.jp factbook)
     stages=dict(inDurSeatB=280, inDur1mmB=248, inLiftB=0.01265, exDurSeatB=278, exDur1mmB=246, exLiftB=0.01214,
                 stageSpeed=6000 * RPM),
     grade="Premium (91)", car="roadster", timing="Chain", lifters="Solid",
     notes="A 2.0 L four with two cam profiles a valve switched at 6000 rpm: 87 x 84 mm (1997 cc), 11.7:1, 9000 rpm "
           "(the F20C kind: 176 kW at 8300 rpm, 208 N m at 7500; makers' figures, from memory, firm to 5 %; the "
           "B16A's switch point of 5500 rpm: global.honda). In a light rear-drive roadster.")
arch(slug="kei-660-triple-turbo", runner=0.28, convK=24.0, name="Kei 660 cc triple turbo", geo="ref-04-inline-3", layout="Inline 3",
     bore=68.0, stroke=60.4, rod=110.0, cr=8.6, valves=4, dIn=24.5, dEx=21.0,
     cam=(240, 208, 7.5, 110, 236, 204, 7.0, 112), train="Bucket", redline=8000, idle=850,
     boost=dict(kind="Turbo", target=0.65, spring=0.10), car="kei",
     boostTaper=[(1500, 65), (4000, 65), (5000, 45), (5250, 36), (6000, 22), (6500, 15), (7000, 12), (8000, 10), (8500, 10)],
     timing="Chain",
     notes="A Japanese kei-class three: 68 x 60.4 mm (658 cc), 8.6:1, four valves, a small turbo at a production 0.65 bar falling to 0.12 bar by 7000 rpm (the class's power cap is met by the ECU's boost taper; the K6A kind: "
           "47 kW (the class's 64 PS cap) at 6500 rpm, 103 to 108 N m at 3500; Wikipedia, goo-net-exchange). In a "
           "780 kg rear-drive sports kei.")
arch(slug="diesel-20-common-rail", governor=(4000, 130000), runner=0.3, name="Diesel 2.0 L common rail", geo="ref-05-inline-4", layout="Inline 4",
     bore=81.0, stroke=95.5, rod=144.0, cr=16.5, valves=4, dIn=27.0, dEx=24.0,
     cam=(228, 196, 8.0, 112, 228, 196, 8.0, 112), train="Finger", redline=4800, idle=800, diesel=True,
     boost=dict(kind="Turbo", target=1.4, spring=1.0, vanes=True), car="family-diesel", timing="Toothed belt",
     notes="A 2.0 L four-valve common-rail diesel with a variable-vane turbo: 81 x 95.5 mm (1968 cc), 16.5:1 (engines "
           "of the kind: 103 to 125 kW at 3500 to 4000 rpm, 320 to 380 N m from 1750; makers' figures). Balance "
           "shafts. In a front-drive family car, six-speed manual.", balance2=True)
arch(slug="tractor-15-inline-six", governor=(1900, 108000), runner=0.4, name="Highway tractor 15 L", geo="ref-10-inline-6", layout="Inline 6",
     bore=137.0, stroke=169.0, rod=262.0, cr=17.0, valves=4, dIn=46.0, dEx=43.0,
     cam=(232, 200, 14.0, 112, 232, 200, 13.0, 112), train="Rocker", redline=2250, idle=700, diesel=True,
     boost=dict(kind="Turbo", target=2.6, spring=2.0, vanes=True), car="tractor", timing="Gears",
     compressionBrake="release",
     notes="A 15 L straight-six truck diesel: 137 x 169 mm (14.9 L), 17:1, one variable-vane turbo, a "
           "compression-release engine brake at its middle stage (the X15 kind: 298 to 451 kW, 1966 to 2779 N m at 900 to 1000 rpm, governed at 1900 to 2100, "
           "idle 600 to 700; cummins.com, international.com); its limiter is the governed no-load speed, 2250 rpm, some 12 % over the rated 2000. In a 36 t tractor-trailer, eighteen speeds.")
# ---- the second batch ----
arch(slug="three-cylinder-10-turbo", runner=0.3, convK=17.0, name="1.0 L three-cylinder turbo", geo="ref-04-inline-3", layout="Inline 3",
     bore=71.9, stroke=82.0, rod=137.0, cr=10.0, valves=4, dIn=27.0, dEx=23.0,
     cam=(232, 200, 8.0, 112, 228, 196, 7.5, 112), train="Finger", phase=(50, 50), redline=6500, idle=800,
     boost=dict(kind="Turbo", target=1.0, spring=0.6), direct=True, car="small-hatch", timing="Toothed belt",
     notes="A downsized 1.0 L three with a small turbo and direct injection: 71.9 x 82 mm (999 cc), 10:1 (engines of "
           "the kind: 74 to 103 kW at 5500 to 6000 rpm, 170 to 205 N m from 1500; makers' figures). In a small "
           "hatch, six-speed manual.")
arch(slug="economy-four-15", runner=0.45, convK=18.0, name="1.5 L economy four", geo="ref-05-inline-4", layout="Inline 4",
     bore=75.0, stroke=84.7, rod=137.0, cr=11.0, valves=4, dIn=28.0, dEx=23.5,
     cam=(232, 200, 8.3, 112, 228, 196, 7.8, 112), train="Finger", phase=(40, 0), redline=6600, idle=750,
     car="compact-cvt", timing="Chain",
     notes="A 1.5 L naturally aspirated four of a compact car: 75 x 84.7 mm (1497 cc), 11:1, an intake cam phaser "
           "(engines of the kind: 80 to 96 kW at 6000 to 6600 rpm, 140 to 155 N m at 4300; makers' figures). "
           "Through a CVT.")
arch(slug="turbo-four-20-gdi", runner=0.3, name="2.0 L turbo four, direct injection", geo="ref-05-inline-4", layout="Inline 4",
     bore=82.5, stroke=92.8, rod=144.0, cr=9.6, valves=4, dIn=33.8, dEx=28.0,
     cam=(240, 208, 10.7, 110, 236, 204, 9.0, 112), train="Finger", phase=(60, 0), redline=6800, idle=750,
     boost=dict(kind="Turbo", target=1.2, spring=0.6, scrolls="Twin scroll"), direct=True, grade="Premium (91)",
     boostTaper=[(1500, 120), (4500, 120), (5300, 105), (6000, 85), (6500, 75), (7000, 70)],
     car="hot-hatch-dct", timing="Chain",
     notes="A 2.0 L direct-injection turbo four: 82.5 x 92.8 mm (1984 cc), 9.6:1 (engines of the kind in hot "
           "hatches: 155 to 228 kW at 5000 to 6500 rpm, 350 to 400 N m from 1600 to 4500; makers' figures). "
           "Its boost is trimmed above 4500 rpm (1.2 bar falling to 0.7), as production two-litres do. Front drive, "
           "six-speed dual clutch.", balance2=True)
arch(slug="v6-35-na", intake=dict(runnerLength=0.50, runnerLength2=0.30, runnerSwitch=4000 * RPM), name="3.5 L V6", geo="ref-09-v6-60", layout="V6", bore=94.0, stroke=83.0, rod=152.0,
     cr=10.8, valves=4, dIn=35.0, dEx=29.0, cam=(240, 208, 10.2, 110, 236, 204, 9.4, 112), train="Finger",
     phase=(40, 30), redline=6700, idle=700, car="family-sedan-auto", timing="Toothed belt",
     notes="A 60-degree 3.5 L V6 of a family car: 94 x 83 mm (3456 cc), 10.8:1, four valves and phased cams (engines "
           "of the kind: 195 to 224 kW at 6200 to 6600 rpm, 340 to 360 N m at 4700; makers' figures). Front drive, "
           "six-speed automatic.")
arch(slug="v6-35-twin-turbo-pickup", runner=0.3, name="3.5 L twin-turbo V6 pickup", geo="ref-09-v6-60", layout="V6",
     bore=92.5, stroke=86.6, rod=152.0, cr=10.5, valves=4, dIn=34.0, dEx=28.0,
     cam=(236, 204, 9.5, 112, 232, 200, 9.0, 112), train="Finger", phase=(50, 40), redline=6000, idle=650,
     boost=dict(kind="Twin turbos", layout="Two, a bank each", target=1.0, spring=0.6), direct=True,
     car="pickup-4wd", timing="Chain",
     notes="A 3.5 L twin-turbo V6 of a full-size pickup: 92.5 x 86.6 mm (3497 cc), 10.5:1, direct injection, a turbo "
           "a bank (engines of the kind: 280 to 300 kW at 5000 to 6000 rpm, 640 to 690 N m at 3500; makers' "
           "figures). Four-wheel drive, ten-speed automatic.")
arch(slug="twin-cam-roadster-18", runner=0.4, convK=17.0, name="1.8 L twin-cam roadster", geo="ref-05-inline-4", layout="Inline 4",
     # the real head's valves and cams (N3, 20:06; the sheets): about 31/26 mm, some 230 to 240 deg at the seat and
     # 8.5 mm lift (from memory, firm to 1 mm and 5 deg)
     bore=83.0, stroke=85.0, rod=133.0, cr=10.0, valves=4, dIn=31.0, dEx=26.0,
     cam=(236, 204, 8.6, 110, 232, 200, 8.3, 112), train="Bucket", redline=7000, idle=850, car="roadster",
     timing="Toothed belt",
     notes="A 1.8 L twin-cam 16-valve four: 83 x 85 mm (1839 cc), 10:1 (engines of the kind in light roadsters: 96 "
           "to 110 kW at 6500 to 7000 rpm, 160 to 170 N m at 5000; makers' figures). Rear drive, six-speed manual.")
arch(slug="pickup-diesel-67-inline-six", governor=(2900, 106000), runner=0.35, name="6.7 L straight-six turbo diesel pickup", geo="ref-10-inline-6",
     layout="Inline 6", bore=107.0, stroke=124.0, rod=192.0, cr=16.2, valves=4, dIn=36.0, dEx=33.0,
     cam=(228, 196, 11.0, 112, 228, 196, 10.5, 112), train="Pushrod", redline=3200, idle=750, diesel=True,
     boost=dict(kind="Turbo", target=2.0, spring=1.6, vanes=True), car="heavy-pickup", timing="Gears",
     compressionBrake=True,
     notes="A 6.7 L straight-six turbo diesel, 107 x 124 mm (6690 cc), 16.2:1, pushrods and four valves, a "
           "variable-vane turbo that doubles as an exhaust brake (engines of the kind: 260 to 310 kW at 2800 to 3000 rpm, 1000 "
           "to 1460 N m at 1600 to 1800; makers' figures). In a heavy-duty pickup, six-speed automatic.")
arch(slug="twin-turbo-hot-vee-v8-40", runner=0.25, name="Twin-turbo 4.0 L hot-vee V8", geo="ref-12-v8-cross",
     layout="V8 crossplane", bore=86.0, stroke=86.0, rod=143.0, cr=10.5, valves=4, dIn=33.0, dEx=28.0,
     cam=(244, 212, 10.5, 110, 240, 208, 9.5, 112), train="Finger", phase=(42, 42), redline=6800, idle=650,
     boost=dict(kind="Twin turbos", layout="Two, a bank each", target=1.2, spring=0.6, scrolls="Twin scroll"),
     direct=True, grade="Premium (91)", car="awd-autobahn", timing="Chain",
     notes="A 4.0 L crossplane V8 with both turbos in the vee (hot-vee): 86 x 86 mm (3996 cc), 10.5:1, direct "
           "injection (engines of the kind: 350 to 470 kW at 5500 to 6500 rpm, 700 to 850 N m from 2000 to 4500; "
           "makers' figures). All-wheel drive, eight-speed automatic.")
arch(slug="vr6-28", runner=0.45, name="Narrow-angle VR6 2.8 L", geo="vr6", layout="Inline 6", bore=81.0, stroke=90.3,
     rod=164.0, cr=10.0, valves=2, dIn=40.0, dEx=35.0, cam=(240, 206, 10.0, 110, 236, 202, 9.5, 112),
     train="Rocker", redline=6500, idle=750, car="hatch-vr", timing="Chain",
     notes="A 15-degree narrow-angle six, one head over both banks, two valves a cylinder: 81 x 90.3 mm (2792 cc), "
           "10:1 (the 12-valve VR6 of 1991: 128 kW at 5800 rpm, 235 N m at 4200; makers' figures, from memory, firm "
           "to 5 %). In a front-drive hatch.")
arch(slug="straight-six-32-high-rev", name="3.2 L straight six, high-revving", geo="ref-10-inline-6",
     layout="Inline 6", bore=87.0, stroke=91.0, rod=139.0, cr=11.5, valves=4, dIn=35.0, dEx=30.5,
     cam=(264, 236, 11.8, 106, 260, 232, 11.0, 110), train="Bucket", phase=(60, 45), redline=8000, idle=850,
     intake=dict(throttleKind="A body a runner"), grade="Premium (91)", car="coupe-six",
     timing="Chain", lifters="Solid",
     notes="A naturally aspirated 3.2 L straight six with a throttle body a cylinder: 87 x 91 mm (3246 cc), 11.5:1, "
           "8000 rpm (the S54 kind: 252 kW at 7900 rpm, 365 N m at 4900; makers' figures, from memory, firm to 5 %). "
           "Rear drive, six-speed manual.")
arch(slug="four-cam-v8-50", runner=0.35, name="Four-cam 5.0 L V8", geo="ref-12-v8-cross", layout="V8 crossplane",
     bore=93.0, stroke=92.7, rod=150.7, cr=12.0, valves=4, dIn=37.0, dEx=31.0,
     cam=(260, 228, 12.0, 108, 256, 224, 11.0, 112), train="Finger", phase=(50, 50), redline=7500, idle=700,
     direct=True, grade="Premium (91)", car="pony-car", timing="Chain",
     notes="A 5.0 L four-cam crossplane V8: 93 x 92.7 mm (5038 cc), 12:1, all four cams phased (engines of the kind: "
           "300 to 343 kW at 6500 to 7000 rpm, 530 to 570 N m at 4250 to 4600; makers' figures). Rear drive, "
           "six-speed manual.")
arch(slug="big-bore-70-pushrod-v8", runner=0.4, name="7.0 L pushrod V8, dry sump", geo="ref-12-v8-cross",
     layout="V8 crossplane", bore=104.8, stroke=101.6, rod=158.0, cr=11.0, valves=2, dIn=56.0, dEx=41.0,
     cam=(282, 230, 15.0, 112, 288, 234, 14.0, 116), train="Pushrod", redline=7000, idle=750,
     friction="Roller", grade="Premium (91)", car="front-engine-sports", timing="Chain", lifters="Hydraulic",
     notes="A 7.0 L small-block-sized pushrod V8 with titanium intake valves and a dry sump: 104.8 x 101.6 mm "
           "(7011 cc), 11:1, 7000 rpm (the LS7 kind: 377 kW at 6300 rpm, 637 N m at 4800; gmauthority.com). "
           "In a light front-engined sports car, six-speed manual.")
arch(slug="pushrod-v6-38", runner=0.45, name="3.8 L pushrod V6", geo="v6-90-split", layout="V6", bore=96.5, stroke=86.4,
     rod=145.0, cr=9.4, valves=2, dIn=46.0, dEx=38.0, cam=(240, 196, 9.4, 112, 244, 200, 9.4, 114), train="Pushrod",
     redline=5700, idle=650, friction="Roller", car="big-sedan-fwd", timing="Chain",
     notes="A 90-degree pushrod V6 with split pins for even firing: 96.5 x 86.4 mm (3791 cc), 9.4:1 (engines of the "
           "kind: 140 to 155 kW at 5200 rpm, 305 to 325 N m at 4000; makers' figures of the 1990s). A big "
           "front-drive sedan, four-speed automatic.", balance1=True)
arch(slug="flat-plane-v8-39-twin-turbo", runner=0.25, name="Flat-plane 3.9 L V8, twin turbo", geo="ref-13-v8-flat",
     layout="V8 flat-plane", bore=86.5, stroke=83.0, rod=148.0, cr=9.4, valves=4, dIn=33.0, dEx=28.0,
     cam=(252, 220, 10.5, 108, 248, 216, 10.0, 112), train="Finger", phase=(40, 40), redline=8000, idle=900,
     boost=dict(kind="Twin turbos", layout="Two, a bank each", target=1.3, spring=0.7, scrolls="Twin scroll"),
     direct=True, grade="Premium (91)", car="mid-engine", timing="Chain",
     notes="A 3.9 L flat-plane V8 with a turbo a bank: 86.5 x 83 mm (3902 cc), 9.4:1, direct injection (engines of "
           "the kind: 450 to 530 kW at 7500 to 8000 rpm, 760 to 800 N m at 3000 to 5750; makers' figures). In a "
           "mid-engined coupe, seven-speed dual clutch.")


# ---- valve springs ------------------------------------------------------------------------------------------------
# Rate (N/m) and preload (N) that put the intake valves' float speed (tel.valve.floatSpeed, card E's
# w_float = sqrt((F0 + k L) / (m |L''|))) 10 % past each engine's limiter: measured with a trial spring of
# 40 kN/m over 250 N, then both scaled by (1.10 x redline / float speed)^2 (w goes as the square root of the force).
# Floored at a soft road spring (25 kN/m over 150 N). The pushrod engines need two to four times a real spring
# (the 7.0 L: 228 kN/m, some 1300 lb/in, where the real one runs some 340): the float model reads pessimistic for a
# pushrod train (reported to card E). The two-stage cam's figures are stage A's: its B profile is sharper, so it
# carries half as much again.
SPRINGS = {
    "air-cooled-flat-six-36": (48500, 303),
    "big-block-74-pushrod-v8": (123900, 774),
    "big-bore-70-pushrod-v8": (228000, 1425),
    "boxer-four-turbo-unequal": (28100, 175),
    "diesel-20-common-rail": (9000, 56),
    "economy-four-15": (17400, 109),
    "flat-plane-v8-39-twin-turbo": (29200, 183),
    "flat-plane-v8-45": (38900, 243),
    "four-cam-v8-50": (30000, 187),
    "hot-hatch-16-16v": (24300, 152),
    "inline-five-turbo-rally": (23600, 147),
    "kei-660-triple-turbo": (25000, 156),
    "pickup-diesel-67-inline-six": (19900, 124),
    "pushrod-v6-38": (88100, 550),
    "small-block-57-carb": (119700, 748),
    "straight-six-32-high-rev": (35900, 225),
    "straight-six-twin-turbo-30": (20200, 126),
    "supercharged-v8-62": (174500, 1090),
    "three-cylinder-10-turbo": (16000, 100),
    "tractor-15-inline-six": (8300, 52),
    "turbo-four-20-gdi": (25000, 156),
    "twin-cam-roadster-18": (25900, 162),
    "twin-turbo-hot-vee-v8-40": (22900, 143),
    "two-stage-cam-20": (54500, 340),
    "v10-52": (34000, 213),
    "v12-65": (41700, 260),
    "v6-35-na": (24300, 152),
    "v6-35-twin-turbo-pickup": (18600, 117),
    "vr6-28": (37900, 237),
}


PORTS = {"wedge": (1.15, 1.3), "high": (1.65, 1.8), "race": (1.75, 1.9), "cnc": (1.6, 1.8)}
# (cnc: the LS7's CNC-ported heads, about 1.6 on N3's bench-flow scale, q = Cd / 0.4 at lift d/4:
# docs/measurements/calibration/balance/discharge.md; the small block's and the 3.8's 1.15 match its 1.15 to 1.2.
# Held off the 7.0 for now (N3, 20:19: it is already some 10 % over; not before N1 settles the runner's ram))
# N1's values with the charge heating and the conserving intake in (measured peak torque over target's middle:
# flat six 1.35, VR6 1.30, 7.0 V8 1.26, pushrod V6 1.25, small block 1.14): stock two-valve wedge and pushrod
# heads 1.15/1.3; a great two-valve (the 7.0, the air-cooled six) and production four-valves at the default
# 1.35/1.5; high-output four-valves 1.65/1.8; the rally five 1.75/1.9
HEADS = {"big-block-74-pushrod-v8": "wedge", "small-block-57-carb": "wedge", "supercharged-v8-62": "wedge",
         "pushrod-v6-38": "wedge", "vr6-28": "wedge", "flat-plane-v8-45": "high",
         "v10-52": "high", "v12-65": "high", "straight-six-32-high-rev": "high", "two-stage-cam-20": "high",
         "flat-plane-v8-39-twin-turbo": "high", "inline-five-turbo-rally": "race"}
for _a in A:
    if _a["slug"] in HEADS:
        _a["ports"] = HEADS[_a["slug"]]
# sports exhausts on the tuned engines (N1: as built every archetype breathes through a road converter and a
# chambered muffler, 154 kPa at the six's port at 7600 rpm against 10 to 30 kPa over the air at rated power)
SPORTS = ("straight-six-32-high-rev", "flat-plane-v8-45", "v12-65", "two-stage-cam-20",
          "flat-plane-v8-39-twin-turbo", "big-bore-70-pushrod-v8", "inline-five-turbo-rally")
for _a in A:
    if _a["slug"] in SPORTS:
        _a.setdefault("exhaust", {}).update({"converter": [2, 2], "mufKind": [1, 1]})
# the torque-peak speeds of each kind (rpm), from the validation's table, for the runners
import importlib.util as _iu
import sys as _sys
_sys.dont_write_bytecode = True    # no __pycache__ beside the validation's generator
_spec = _iu.spec_from_file_location("_val", os.path.join(ROOT, "content", "validation", "archetypes", "generate.py"))
_val = _iu.module_from_spec(_spec)
_spec.loader.exec_module(_val)
TORQUE_PEAK = {k: v[3] for k, v in _val.T.items()}

# ---- writing them ------------------------------------------------------------------------------------------------
def build(a):
    e = {"layout": a["layout"], "custom": True, "compression": a["cr"]}
    e.update(geometry(a["geo"]))
    n = e["cylinders"]
    if a["layout"] == "Rotary":
        e["displacement"] = a["displacement"]
        e["boreStroke"] = 1.0
    else:
        b, s = a["bore"] / 1000.0, a["stroke"] / 1000.0
        e["displacement"] = round(math.pi / 4 * b * b * s * n, 7)
        e["boreStroke"] = round(b / s, 4)
        e["bore"], e["stroke"] = b, s
        e["rodLength"] = a["rod"] / 1000.0
    e["redlineSpeed"] = round(a["redline"] * RPM, 3)
    e["frictionValvetrain"] = a.get("friction", "Finger follower" if a.get("train") in ("Finger", "Bucket", "Rocker")
                                    else "Flat tappet")
    if a.get("balance1"):
        e["balance1"] = True
    if a.get("balance2"):
        e["balance2"] = True
    e["rough"] = 0.2
    e["benchLoad"] = 0.3

    v = {"model": "Lab"}
    if "cam" in a:
        di, d1, li, ci, de, d1e, le, ce = a["cam"]
        v.update(valves=a["valves"], dIn=a["dIn"] / 1000.0, dEx=a["dEx"] / 1000.0, train=a["train"],
                 inDurSeat=di, inDur1mm=d1, inLift=li / 1000.0, inCentre=ci,
                 exDurSeat=de, exDur1mm=d1e, exLift=le / 1000.0, exCentre=ce)
        if "phase" in a:
            v["inPhaseRange"], v["exPhaseRange"] = a["phase"]
            # a phased intake parks late, as real phasers lock at their retarded end, and advances under load:
            # the fixed-cam centre plus 20 degrees, at most half its range (N1: the four-cam 5.0 parked at its fixed
            # centre had 38 degrees of overlap at idle and hunted 455 to 515 rpm; 20 later it idles 700, sd 6)
            v["inCentre"] = ci + min(20.0, a["phase"][0] / 2.0)
        # port quality by head (N1, from card E's calibration: q 1.35 is a stock 232 CFM small-block head; racing
        # ports flow 20 to 30 % more, Heywood fig. 6-18): two-valve wedge and pushrod heads 1.2/1.35, production
        # four-valve 1.35/1.5 (the default), high-output four-valve 1.65/1.8, race 1.75/1.9
        if a.get("ports"):
            v["portIn"], v["portEx"] = PORTS[a["ports"]]
        if a["slug"] in SPRINGS:
            k, f0 = SPRINGS[a["slug"]]
            m = 1.5 if "stages" in a else 1.0
            v["springRate"], v["springPreload"] = max(k * m, 25000.0), max(f0 * m, 150.0)
        if "stages" in a:
            v["stages"] = "On"
            v.update(a["stages"])
            v.setdefault("inCentreB", ci)
            v.setdefault("exCentreB", ce)
        # a compression-release brake's lobe (the ECU decides when it acts; valve.auxOn stays Off: card D2's)
        if a.get("compressionBrake") == "release":
            v.update(auxOpen=695, auxDur=100, auxLift=0.0025)
    else:
        # a rotary's ports by the cam dial: a peripheral-exhaust turbo rotary mid-way (0.55); a side-exhaust-port
        # rotary has almost no overlap: 0.15 (N1: 47 degrees of overlap at 0.55 cost it a quarter of its torque)
        v["cam"] = a.get("rotaryCam", 0.55)
        if "rotaryPorts" in a:
            # the ports' own timing, as free events in four-stroke-equivalent degrees (x1.5 the eccentric shaft's;
            # N1's reading of the rotary's port fields): (exhaust opens, closes, intake opens, closes)
            eo, ec, io, ic = a["rotaryPorts"]
            v.update(inKind="Free valve", exKind="Free valve", freeExOpen=eo, freeExClose=ec, freeInOpen=io,
                     freeInClose=ic)

    # the runners' length: what the kind has (long for low-speed torque, short for high revs, two lengths where the
    # real ones had them): the loaded-pipe relation x tan x = A L / V puts the ram's bump where the kind wants it
    i = {"model": 1}
    if "runner" in a:
        i["runnerLength"] = a["runner"]
    # A part never comes from the same engine's target rows (the lead, 20:46: the old rule here, 0.5 x (5000 / the
    # target's torque-peak rpm)^1.3, tuned each runner to the very speed its torque row is read at). A naturally
    # aspirated road engine without a runner of its own takes the class default: 0.40 m from the valve to the plenum
    # (judgement; the one figure found, the LS7's manifold runner some 280 mm on its centreline plus its port, a forum's
    # measurement, agrees). No source gave the seven cited engines' own (searched 24 Nov: LS7, VR6 AAA, 3800 L36,
    # Coyote 3, MX-5 BP, the 964's two-plenum system).
    if "boost" not in a and a["layout"] != "Rotary" and "runner" not in a and "runnerLength2" not in a.get("intake", {}):
        i["runnerLength"] = 0.40
    i.update(a.get("intake", {}))
    rule_runner = "runnerLength" not in a.get("intake", {}) and a["layout"] != "Rotary"
    # the naturally aspirated engines' runners by their manifold's class (the lead, 20:52), over any length set by
    # hand above: written down before any table was run on them, and never from the engine's own rows
    if a["slug"] in MANIFOLD:
        cls = MANIFOLD[a["slug"]]
        for k_ in ("runnerLength2", "runnerSwitch"):
            i.pop(k_, None)
        i["runnerLength"] = RUNNER[cls][0]
        if a["slug"] in SWITCHED:
            i["runnerLength2"] = round(0.65 * RUNNER[cls][0], 2)
            i["runnerSwitch"] = round(0.65 * a["redline"] * RPM, 1)
        rule_runner = False

    bo = {"model": "Machine", "kind": "Natural", "level": 0.0, "blowOff": 0.7}
    if "boost" in a:
        k = a["boost"]
        bo["kind"] = k["kind"]
        bo["size"] = 1.0
        bo["level"] = round(min(max((k["target"] - (0.35 if k["kind"] != "Supercharger" else 0.25))
                                    / (1.05 if k["kind"] != "Supercharger" else 0.75), 0.0), 1.0), 3)
        if "layout" in k:
            bo["layout"] = k["layout"]
        bo["target"] = k["target"] * BAR
        if k["kind"] == "Supercharger":
            bo["charger"] = k["charger"]
            bo["scDisplacement"] = k.get("scDisplacement", 0)
            bo["scBypass"] = True
        else:
            bo["gateSpring"] = k.get("spring", 0.7) * BAR
            bo["blowOffKind"] = k.get("blowOff", "Recirculating" if not a.get("diesel") else "None")
            if k.get("scrolls"):
                bo["scrolls"] = k["scrolls"]
            if k.get("vanes"):
                bo["vanes"] = "Variable vanes"
            if k.get("antiLag"):
                bo["antiLag"] = True
            if a.get("gateDiameter"):
                bo["gateDiameter"] = a["gateDiameter"]
            bo["icEffect"] = 0.75
        # the intercooler sized for the engine's air and the charge pipes for its size (N2, 19:55: the defaults, 0.25
        # kg/s and 5 L, suit 2 to 3 L); the pipes twice the swept volume, 1 to 20 L
        # (from the engine's own hardware, never its power row: the air it swallows at its limiter, VE 0.9, at its
        # boost target over the air)
        swept = a["displacement"] if "displacement" in a else n * math.pi / 4 * (a["bore"] / 1000) ** 2 * a["stroke"] / 1000
        air = 1.2 * swept * a["redline"] / 120.0 * 0.9 * (1.0 + k["target"])
        bo["icFlow"] = round(min(max(air, 0.05), 2.0), 3)
        bo["chargeVolume"] = round(min(max(2.0 * a["displacement"] if "displacement" in a else
                                           2.0 * n * math.pi / 4 * (a["bore"] / 1000) ** 2 * a["stroke"] / 1000,
                                           0.001), 0.02), 4)
    else:
        bo["model"] = "Machine"

    x = {"model": "Builder", "sides": 2 if n >= 6 and a["layout"] not in ("Inline 6",) else 1, "converter": [1, 1], "mufKind": [2, 2]}
    if a["slug"] in EXHAUST_CLASS:
        x["headers"] = HEADER[EXHAUST_CLASS[a["slug"]]][0]
    x.update(a.get("exhaust", {}))

    diesel = a.get("diesel", False)
    f = {"kind": "Diesel" if diesel else "Gasoline", "model": "Fuels",
         "grade": "Diesel" if diesel else a.get("grade", "Regular (87)")}
    if a.get("direct") or diesel:
        f["direct"] = 1

    t = {"model": "Maps", "timing": 0, "pops": 0.1, "starter": True,
         "limiter.kind": a.get("limiter", "Fuel cut"), "limiter.speed": e["redlineSpeed"],
         "idle.speed": round(a["idle"] * RPM, 3)}
    if "governor" in a:
        # a diesel's rack-metered governor (card D2's): full fuel to the rated speed, the rack's stroke set by the boost
        # it is matched to (D2's measured values put each at its maker's rated power, lambda about 1.5)
        rated, fuel_boost = a["governor"]
        t["governor"] = "Electronic"
        t["ratedSpeed"] = round(rated * RPM, 1)
        t["fuelBoost"] = fuel_boost
        # its limit is the governor's high idle, the rated speed plus its droop (0.06): the limiter and the redline there
        e["redlineSpeed"] = round(rated * 1.06 * RPM, 3)
        t["limiter.speed"] = e["redlineSpeed"]
    if a.get("compressionBrake") == "release":
        # a compression-release engine brake (card D2's: the ECU opens the exhaust near the compression top with the
        # pedal up and the clutch in, above ecu.brakeMin), its stage switch at Medium; the lobe as D2's trucks have it
        t["engineBrake"] = "Compression release"
        t["brakeSwitch"] = "Medium"
    elif a.get("compressionBrake"):
        # the exhaust brake by the turbine's vanes, armed on the dash (card D2's), as most 6.7 L pickups have it
        t["exhaustBrake"] = "Turbine vanes"
        t["exBrakeSwitch"] = "On"
    if a.get("antiLag"):
        t["antiLag.on"] = True
        t["launch.on"] = True
    if CARS[a["car"]].get("launch"):
        t["launch.on"] = True
    if "boost" in a:
        t["knock.on"] = True
    if a["slug"] in CAMS:
        # the maker's calibrated intake cam map, baked by cams.py (C's tee-dyno --bake-cam: the cam swept at full load,
        # the best advance by speed), the same full-load row at no load and at three times the air (Bake.cpp's form)
        pts = CAMS[a["slug"]][:16]
        t["maps.camInAuto"] = False
        t["maps.camIn"] = {"x": [w for w, _ in pts], "y": [0.0, 3.0], "z": [[adv, adv] for _, adv in pts]}
    if "boostTaper" in a:
        # the class's power cap met as its makers meet it: the boost target falls with speed (a baked boost map,
        # every gear alike; rpm and kPa over the air from N2's measurements on the kei with the resized gate)
        xs = [round(r * RPM, 2) for r, _ in a["boostTaper"]]
        gears = list(range(1, len(CARS[a["car"]]["ratio"]) + 1))
        t["maps.boostAuto"] = False
        t["maps.boost"] = {"x": xs, "y": gears, "z": [[k * 1000.0] * len(gears) for _, k in a["boostTaper"]]}

    # the mechanical voices (package M) on, with what belongs to this engine (the lead, 20:09: every model switch on)
    vo = {"model": "Voices", "timing": a.get("timing", "Chain")}
    if a.get("lifters"):
        vo["lifters"] = a["lifters"]
    if a.get("fan"):
        vo["fan"] = a["fan"]
    if a.get("blowerDrive"):
        vo["blowerDrive"] = a["blowerDrive"]
    vo["injection"] = "Direct" if a.get("direct") and not diesel else "Port"

    car = car_section(a["car"])
    # a torque converter matched to the engine (used by an automatic, and by the drag and top-speed runs that make
    # the gearbox one): the default's capacity (13.5) suits a big engine and loads a small one's idle with some 40 N m;
    # small engines' converters are looser: K grows as the square root of the torque falls (judgement, from the
    # converter's law T = (w / K)^2)
    if "convK" in a:
        car["converterK"] = a["convK"]
    # a clutch sized on the engine's measured peak with a margin, where the car's own estimate (1.35 x the engine's
    # estimated peak) slips under the real torque (the lead, 19:36: the NA rotary's after its launch)
    if "clutchTorque" in a:
        car["clutchTorque"] = a["clutchTorque"]
    # Woschni's walls, said outright (fuel.model 1 brings them anyway: engine.fm's walls()); four strokes
    e["wallHeat"] = "Woschni"
    e["cycle"] = "Four-stroke"
    # package L's listener on: every source where it is on the car; the first camera behind the car in the open,
    # the engine where the class carries it
    snd = {"model": "Places", "camera": "Rear", "place": "Field", "ground": "Asphalt",
           "enginePlace": ENGINE_PLACE.get(a["car"], "Front")}
    spec = {"engine": e, "valve": v, "intake": i, "boost": bo, "exhaust": x, "fuel": f, "ecu": nest(t),
            "car": car, "env": {"temperature": 288.15, "pressure": 101325.0}, "voices": vo, "sound": snd}
    switches_on(spec, a["slug"])
    notes = a["notes"]
    if a["slug"] in EXHAUST_CLASS:
        hc = EXHAUST_CLASS[a["slug"]]
        notes += (" Its exhaust: %s, the primaries %.2f times the layout's length: the class the real engine carries "
                  "(recalled), its length JUDGEMENT." % (HEADER[hc][1], math.exp(0.9808293 * (HEADER[hc][0] - 0.5))))
    if a["slug"] in CAMS:
        notes += " Its intake cam map is baked (the cam swept at full load, the best advance by speed: cams.py)."
    if a["slug"] in MANIFOLD:
        cls = MANIFOLD[a["slug"]]
        notes += (" Its intake: the %s class's runner, %.2f m from the valve to the plenum (%s)%s: JUDGEMENT, a class "
                  "default, not the real engine's (no source found)." % (
                      cls, RUNNER[cls][0], RUNNER[cls][1],
                      "; a second, short runner of %.2f m above %.0f rpm, as the real engine switches (%s; its "
                      "lengths and speed the class's judgement)" % (i["runnerLength2"], i["runnerSwitch"] / RPM,
                                                                    SWITCHED[a["slug"]])
                      if a["slug"] in SWITCHED else ""))
    elif rule_runner and "runnerLength" in i and "JUDGEMENT" not in notes:
        notes += (" Its intake runner, %.2f m from the valve to the plenum, is %s: JUDGEMENT, not the real engine's (no "
                  "source found)." % (i["runnerLength"], "a hand-set length" if "runner" in a else "the class default"))
    return {"format": "tee-build", "version": 1, "name": a["name"], "notes": notes, "spec": spec}


# (no per-build sound.level: L's auto level on model 1 sets a free rev near 0.4 to 0.55 and the idle near 0.1,
# c686d345, the lead's ruling; docs/models/listen.md, "Auto level")

# The naturally aspirated engines' intake runners by the class of their manifold (the lead, 20:52), valve to plenum,
# JUDGEMENT, written down before any table ran on them (24 Nov, 20:55 UTC) and never chosen from an engine's rows;
# a second, switched runner only where the real engine switches (0.65 of the class's length above 0.65 of the limiter:
# judgement). The class goes by the manifold the real engine has, not by its targets.
RUNNER = {
    "carburettor": (0.20, "a four-barrel over a dual-plane or open plenum, 1960s to 80s"),
    "long-runner": (0.45, "a port-injected road engine's tuned long runners, 1985 to 2005"),
    "modern": (0.38, "a road engine's composite plenum manifold, 2005 on"),
    "sports": (0.30, "a high-revving engine's short runners or a throttle a cylinder"),
}
MANIFOLD = {
    "big-block-74-pushrod-v8": "carburettor", "small-block-57-carb": "carburettor",
    "air-cooled-flat-six-36": "long-runner", "vr6-28": "long-runner", "pushrod-v6-38": "long-runner",
    "twin-cam-roadster-18": "long-runner", "hot-hatch-16-16v": "long-runner",
    "big-bore-70-pushrod-v8": "modern", "four-cam-v8-50": "modern", "economy-four-15": "modern",
    "v6-35-na": "modern",
    "straight-six-32-high-rev": "sports", "two-stage-cam-20": "sports", "flat-plane-v8-45": "sports",
    "v10-52": "sports", "v12-65": "sports",
}
SWITCHED = {
    "v6-35-na": "the 2GR-FE's ACIS, a valve in the upper manifold closed at low and middle speeds (toyota-club.net; "
                "service-manual mirrors give some 3600 to 4450 rpm, unverified)",
    "flat-plane-v8-45": "the 458's three throttle valves between its two plenums giving four configurations, with "
                        "short, almost straight tracts (Ferrari's launch text, pistonheads.com): a plenum's geometry "
                        "switched, here as a second runner",
}

# The exhaust headers by the class the real engine carries (the lead, 21:05), JUDGEMENT, written down before any table
# ran on them (24 Nov, 21:15 UTC) and never chosen from an engine's rows: exhaust.headers, the primaries 0.61 (0), 1.0
# (0.5) or 1.63 (1) times the layout's length. The class goes by the real engine's manifold (recalled), not by targets.
HEADER = {
    "log": (0.0, "a cast or short log manifold"),
    "tubular": (0.5, "short tubular headers"),
    "tuned": (1.0, "tuned long-primary headers"),
}
EXHAUST_CLASS = {
    "big-block-74-pushrod-v8": "log", "small-block-57-carb": "log", "pushrod-v6-38": "log", "vr6-28": "log",
    "economy-four-15": "log", "v6-35-na": "log",
    "boxer-four-turbo-unequal": "log", "inline-five-turbo-rally": "log", "kei-660-triple-turbo": "log",
    "three-cylinder-10-turbo": "log", "turbo-four-20-gdi": "log", "straight-six-twin-turbo-30": "log",
    "flat-plane-v8-39-twin-turbo": "log", "twin-turbo-hot-vee-v8-40": "log", "v6-35-twin-turbo-pickup": "log",
    "diesel-20-common-rail": "log", "pickup-diesel-67-inline-six": "log", "tractor-15-inline-six": "log",
    "air-cooled-flat-six-36": "tubular", "big-bore-70-pushrod-v8": "tubular", "four-cam-v8-50": "tubular",
    "twin-cam-roadster-18": "tubular", "hot-hatch-16-16v": "tubular", "supercharged-v8-62": "tubular",
    "flat-plane-v8-45": "tuned", "v10-52": "tuned", "v12-65": "tuned", "two-stage-cam-20": "tuned",
    "straight-six-32-high-rev": "tuned",
}


def _cams():
    """Each phased archetype's baked intake cam map (cams.py: tee-dyno --bake-cam), {slug: [(rad/s, advance)]}.
    NOT USED (the lead, 22:35): a baked map without a load axis asked the full-load phase at idle (the 4.5 idled at
    50 deg and hunted; first.json at 20); the maps go back to N1 until one has a load axis, is smooth across speed and
    passes the idle check. Every phased archetype keeps camInAuto (N1's rule)."""
    return {}
    path = os.path.join(HERE, "cam-maps.tsv")
    out = {}
    if os.path.exists(path):
        for line in open(path):
            if line[:1] == "#":
                continue
            slug, w, adv = line.rstrip("\n").split("\t")
            out.setdefault(slug, []).append((float(w), float(adv)))
    return out


CAMS = _cams()

# where each class carries its engine (the listener's paths to each camera)
ENGINE_PLACE = {"mid-engine": "Mid", "awd-supercar": "Mid", "rear-engine-classic": "Rear"}


def _switches():
    """Every model switch the Schema has (each spec field named `model`, and engine.wallHeat), from tee-schema --json;
    None where the tool is not built."""
    tool = os.environ.get("TEE_SCHEMA", os.path.join(ROOT, "build-dev", "bin", "tee-schema"))
    if not os.path.exists(tool):
        return None
    import subprocess
    d = json.loads(subprocess.run([tool, "--json"], capture_output=True, text=True).stdout)
    return {f["path"][len("spec."):]: f.get("choices") for f in d["fields"]
            if f["path"].startswith("spec.") and f["path"].split(".")[-1] in ("model", "wallHeat")}


SWITCHES = _switches()


def switches_on(spec, slug):
    """Fails unless every model switch the Schema has is set on (not its first choice, the reference) in `spec`: so
    "every switch on" is true by construction (the lead, 20:09). Warns and passes where tee-schema is not built."""
    if SWITCHES is None:
        print("warning: tee-schema not built: the model switches are not checked")
        return
    for path, choices in SWITCHES.items():
        sec, field = path.split(".", 1)
        val = spec.get(sec, {}).get(field)
        if val is None or val == 0 or (choices and val == choices[0]):
            raise SystemExit("%s: spec.%s is not on (%r): every archetype sets every model switch" % (slug, path, val))


def nest(flat):
    out = {}
    for k, val in flat.items():
        if "." in k:
            a, b = k.split(".", 1)
            out.setdefault(a, {})[b] = val
        else:
            out[k] = val
    return out


def write(path, doc):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump(doc, f, indent=2)
        f.write("\n")


FIRST = "turbo-four-20-gdi"
ENGINE_SECTIONS = ("engine", "valve", "intake", "boost", "exhaust", "fuel")


def idle_check(docs):
    """Every archetype idles within its band and steadily, warm and cold, before any file is written (the lead, 22:35:
    a baked cam map asked the full-load phase at idle and the 4.5 hunted). On a tee-render of p-idle-warm, 8 to 14 s:
    the mean within 100 rpm of the speed its ECU asks and the standard deviation under 25 rpm; of p-cold-start, 8 to
    11.5 s: running (engine.state 2) and the deviation under 60 rpm (judgement: a cold fast idle falls as it warms).
    Uses $TEE_BIN (default build-dev/bin) and $TEE_SIM (default the repository's sim/); --no-check skips it, loudly.
    Returns the failures as lines."""
    import csv
    import statistics
    import subprocess
    import tempfile
    from concurrent.futures import ThreadPoolExecutor
    tool = os.path.join(os.environ.get("TEE_BIN", os.path.join(ROOT, "build-dev", "bin")), "tee-render")
    if not os.path.exists(tool):
        return ["tee-render not built (%s): the idle check cannot run; build it, or pass --no-check" % tool]
    sim = os.environ.get("TEE_SIM", os.path.join(ROOT, "sim"))
    tmp = tempfile.mkdtemp(prefix="idle-check-")
    scen = os.path.join(ROOT, "content", "validation", "scenarios")

    def one(item):
        slug, doc = item
        path = os.path.join(tmp, slug + ".json")
        with open(path, "w") as f:
            json.dump(doc, f)
        asked = doc["spec"]["ecu"]["idle"]["speed"] / RPM
        bad = []
        for name, t0, t1 in (("p-idle-warm", 8.0, 14.0), ("p-cold-start", 8.0, 11.5)):
            out = os.path.join(tmp, slug + "-" + name)
            subprocess.run([tool, "--build", path, "--scenario", os.path.join(scen, name + ".json"), "--out",
                            out + ".wav", "--tel", "engine.speed,engine.state", "--sim", sim], capture_output=True)
            try:
                rows = [r for r in csv.DictReader(open(out + ".csv")) if t0 <= float(r["time"]) <= t1]
            except OSError:
                bad.append("%s: %s did not render" % (slug, name))
                continue
            rpm = [float(r["engine.speed"]) / RPM for r in rows]
            sd = statistics.pstdev(rpm) if rpm else 0.0
            mean = statistics.mean(rpm) if rpm else 0.0
            if name == "p-idle-warm" and (abs(mean - asked) > 100 or sd > 25):
                bad.append("%s: warm idle %.0f rpm (asked %.0f), sd %.0f" % (slug, mean, asked, sd))
            if name == "p-cold-start" and (min(float(r["engine.state"]) for r in rows) < 2 or sd > 60):
                bad.append("%s: cold idle %.0f rpm, sd %.0f, %s" % (
                    slug, mean, sd, "running" if min(float(r["engine.state"]) for r in rows) >= 2 else "NOT running"))
        return bad
    with ThreadPoolExecutor(int(os.environ.get("TEE_JOBS", "4"))) as ex:
        return [line for lines in ex.map(one, docs) for line in lines]


def main():
    import sys
    docs = [(a["slug"], build(a)) for a in A]
    if "--no-check" in sys.argv:
        print("WARNING: the idle check skipped (--no-check): these files are not known to idle")
    else:
        failures = idle_check(docs)
        if failures:
            raise SystemExit("the idle check failed; nothing written:\n  " + "\n  ".join(failures))
        print("idle check: %d archetypes idle within their bands, warm and cold" % len(docs))
    built = dict(docs)
    for a in A:
        doc = built[a["slug"]]
        write(os.path.join(HERE, a["slug"] + ".json"), doc)
        eng = {k: doc["spec"][k] for k in ENGINE_SECTIONS}
        write(os.path.join(PARTS, "engines", a["slug"] + ".json"),
              {"format": "tee-build", "version": 1, "part": "engine", "name": a["name"] + " (engine)",
               "notes": "The engine of the archetype \"" + a["name"] + "\" alone, to put in another car.",
               "spec": eng})
        write(os.path.join(PARTS, "tunes", a["slug"] + ".json"),
              {"format": "tee-build", "version": 1, "part": "tune", "name": a["name"] + " (tune)",
               "notes": "The tune of the archetype \"" + a["name"] + "\" alone.", "spec": {"ecu": doc["spec"]["ecu"]}})
    for name in CARS:
        write(os.path.join(PARTS, "cars", name + ".json"),
              {"format": "tee-build", "version": 1, "part": "car", "name": "Car: " + name.replace("-", " "),
               "notes": "A car of its class alone (card P's archetypes' cars), to take any engine.",
               "spec": {"car": car_section(name)}})
    # the build the application opens on a first launch (the lead, 20:06; B opens content/builds/first.json when
    # there is no --preset and no remembered build): an archetype inside its power, torque, idle and warm-start rows,
    # steady at idle, cheap to run, and fun in the first minute
    first = build(next(a for a in A if a["slug"] == FIRST))
    # (the 2.0 turbo four: Q measured the 3.0 six at 1.46 R at Full before the listener and the voices, over the 1.4
    # the lead asked; the four 0.92 R)
    first["name"] = "2.0 L turbo four"
    first["notes"] = ("Try first: turn the key, then floor it on the dyno and watch the turbo spool and the wastegate "
                      "open in AIR, and the four pulses run down the exhaust in EXHAUST.\n"
                      "Then drive: in first, hold the brake, press the launch button, floor the pedal and let go for a "
                      "launch-control start (traction control holds the wheelspin), and listen to the direct injection's "
                      "tick and the turbo's whistle as the dual clutch shifts.\n\n" + first["notes"])
    # the Tailpipe camera, model 1's form of the reference's default view, what the owner heard before first.json
    # (the lead's ruling, 22:53, on his renders and L's: within 1 to 5 dB of the reference in every octave; Side was
    # 9 dB down at 500 Hz and up in the highs, Rear 7 down at 1 kHz)
    first["spec"]["sound"]["camera"] = "Tailpipe"
    write(os.path.join(ROOT, "content", "builds", "first.json"), first)
    loose_parts(A)
    manual(A)
    print("%d archetypes, %d cars" % (len(A), len(CARS)))


EXHAUSTS = [
    ("stock-quiet", "Stock exhaust, quiet", "A road car's system: medium headers, a resonator and a three-pass muffler on each side.",
     {"headers": 0.5, "pipe": 0.5, "crossover": 0.0, "muffler": 0.85},
     {"converter": [1, 1], "resonator": [2, 2], "mufKind": [3, 3]}),
    ("sport-cat-back", "Sport cat-back", "Straight-through mufflers behind the standard converters: louder, a harder edge.",
     {"headers": 0.5, "pipe": 0.5, "crossover": 0.0, "muffler": 0.35},
     {"converter": [1, 1], "resonator": [0, 0], "mufKind": [1, 1]}),
    ("long-tube-x-pipe", "Long-tube headers and an X-pipe", "Long primaries for mid-range torque, both sides crossed in an X "
     "near the collectors, chambered mufflers.", {"headers": 0.85, "pipe": 0.5, "crossover": 0.6, "muffler": 0.45},
     {"converter": [2, 2], "crossKind": 2, "crossAt": 0, "mufKind": [2, 2]}),
    ("h-pipe-dual", "Dual exhaust with an H-pipe", "Two sides joined by an H-pipe: the deeper, classic V8 note.",
     {"headers": 0.6, "pipe": 0.55, "crossover": 0.5, "muffler": 0.5},
     {"converter": [1, 1], "crossKind": 1, "crossAt": 1, "mufKind": [2, 2]}),
    ("open-race", "Open race pipes", "No converters and no mufflers: short headers into open pipes. Loud.",
     {"headers": 0.35, "pipe": 0.25, "crossover": 0.0, "muffler": 0.0},
     {"converter": [0, 0], "resonator": [0, 0], "mufKind": [0, 0]}),
    ("truck-stack", "Truck stack", "A long single pipe to a vertical stack behind the cab, one big muffler.",
     {"headers": 0.5, "pipe": 0.95, "crossover": 0.0, "muffler": 0.6},
     {"sides": 1, "converter": [1, 1], "mufKind": [2, 2], "exitAt": [6, 6]}),
]


def loose_parts(archs):
    """Cams, turbos and exhausts alone: files without a "part", which GARAGE applies field by field (only what they
    set), so a cam goes into an engine without resetting the rest of it."""
    for a in archs:
        b = build(a)
        v = b["spec"]["valve"]
        if "inDurSeat" in v:
            cam = {k: v[k] for k in v if k not in ("valves", "dIn", "dEx")}
            write(os.path.join(PARTS, "cams", a["slug"] + ".json"),
                  {"format": "tee-build", "version": 1, "name": a["name"] + " cams",
                   "notes": "The cams, springs and phasers of the archetype \"%s\" alone (intake %d/%d degrees at the "
                            "seat/1 mm, %.1f mm; exhaust %d/%d, %.1f mm): loaded as a part, they replace only the "
                            "valvetrain's cam fields." % (a["name"], v["inDurSeat"], v["inDur1mm"], v["inLift"] * 1000,
                                                          v["exDurSeat"], v["exDur1mm"], v["exLift"] * 1000),
                   "spec": {"valve": cam}})
        bo = b["spec"]["boost"]
        if bo["kind"] != "Natural":
            write(os.path.join(PARTS, "turbos" if bo["kind"] != "Supercharger" else "superchargers",
                               a["slug"] + ".json"),
                  {"format": "tee-build", "version": 1, "name": a["name"] + ": " + bo["kind"].lower(),
                   "notes": "The forced induction of the archetype \"%s\" alone (%s, %.2f bar); loaded as a part it "
                            "replaces only the boost fields it sets. Its sizes are matched to whatever engine it goes "
                            "on." % (a["name"], bo["kind"].lower(), bo["target"] / BAR),
                   "spec": {"boost": bo}})
    for slug, name, notes, dials, builder in EXHAUSTS:
        x = dict(dials)
        x["model"] = "Builder"
        x.update(builder)
        write(os.path.join(PARTS, "exhausts", slug + ".json"),
              {"format": "tee-build", "version": 1, "name": name,
               "notes": notes + " Loaded as a part, it replaces only the exhaust fields it sets. Today the engine "
                                "hears the four dials (headers, pipe, crossover, muffler); the builder's pipes are "
                                "kept for when it runs them.",
               "spec": {"exhaust": x}})


def manual(archs):
    """docs/manual/builds.md: the archetypes in a table, then each one's notes."""
    out = ["# The shipped builds", "",
           "*Generated by `content/builds/archetypes/generate.py`: do not edit by hand.*", "",
           "Thirty archetypes in `content/builds/archetypes/`, each a whole build (engine, car, tune) that starts, idles, "
           "pulls through the gears and stops, with every part of the model switched on and standard-day air (15 C, "
           "101.3 kPa). Their names say what they are, not whose they are; the dimensions are published figures of "
           "engines of each kind, named in each build's notes. The engines, cars and tunes are also in "
           "`content/builds/parts/` to mix: an engine in another car, a tune on another engine.", "",
           "| Build | Engine | Bore x stroke | Compression | Aspiration | Redline | Car | Gearbox |",
           "|---|---|---|---|---|---|---|---|"]
    for a in archs:
        b = build(a)
        e, c, bo = b["spec"]["engine"], b["spec"]["car"], b["spec"]["boost"]
        bs = "%.1f x %.1f mm" % (a["bore"], a["stroke"]) if "bore" in a else "two rotors"
        asp = bo["kind"] if bo["kind"] == "Natural" else "%s, %.1f bar" % (bo["kind"], bo["target"] / BAR)
        out.append("| %s | %.2f L %s | %s | %.1f:1 | %s | %d rpm | %s, %.0f kg | %s, %d gears |" % (
            a["name"], e["displacement"] * 1000, a["layout"], bs, a["cr"], asp, a["redline"],
            c["drive"] + "-drive", c["mass"], c["gearbox"], c["gears"]))
    out += ["", "## Each build", ""]
    for a in archs:
        out += ["**%s** (`%s.json`). %s" % (a["name"], a["slug"], a["notes"]), ""]
    out += ["## The other packages' builds", "",
            "Made by the packages that built each part of the simulator, to show or test their part (motorcycles, "
            "two-strokes, layouts, cams, turbos, the reference's converted presets). Not all of them are whole "
            "machines: some set one section and leave the rest at its defaults.", "",
            "| File | Name | Notes |", "|---|---|---|"]
    base = os.path.join(ROOT, "content", "builds")
    for dirpath, dirs, files in sorted(os.walk(base)):
        dirs.sort()
        rel = os.path.relpath(dirpath, base)
        if rel.split(os.sep)[0] in ("archetypes", "parts"):
            continue
        for fn in sorted(files):
            if not fn.endswith(".json"):
                continue
            try:
                with open(os.path.join(dirpath, fn)) as f:
                    d = json.load(f)
            except ValueError:
                continue
            note = d.get("notes", "").replace("|", "/").replace("\n", " ")
            note = note if len(note) < 160 else note[:157].rsplit(" ", 1)[0] + " ..."
            out.append("| `%s` | %s | %s |" % (os.path.normpath(os.path.join(rel, fn)), d.get("name", fn[:-5]), note))
    with open(os.path.join(ROOT, "docs", "manual", "builds.md"), "w") as f:
        f.write("\n".join(out))


if __name__ == "__main__":
    main()
