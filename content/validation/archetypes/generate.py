#!/usr/bin/env python3
"""Writes card P's validation targets for each archetype (content/validation/archetypes/<slug>.json).

Run from the repository's root after content/builds/archetypes/generate.py:
    python3 content/validation/archetypes/generate.py
    build-dev/bin/tee-validate --simulator content/validation/archetypes/<slug>.json

Every target names its source and how firm it is. Only the invariants (it starts; within full scale; it holds its
limiter; the car moves; it stops at the key) are hard; the rest are reported: their misses are the simulator's to-do
list (docs/manual/validation.md has the table). The published figures are each archetype's kind's, as its build's
notes say; ranges are wide enough to take the kind's spread, not one engine's brochure.
"""
import json
import math
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
BUILDS = os.path.join(ROOT, "content", "builds", "archetypes")
RPM = math.pi / 30.0
SC = "validation/scenarios/"

# each target's kind of source (the invariants: cited, the contracts and the build's own figures)
BASIS = {
    "Starts cold": "judgement", "Starts hot": "judgement", "Hot idle": "judgement",
    "Sound: a free rev is loud enough": "judgement", "Sound: the idle is audible": "judgement",
    "Peak power": "recalled", "Speed at peak power": "recalled", "Peak torque": "recalled",
    "Speed at peak torque": "recalled",
    "Warm idle": "recalled", "Idle steadiness": "judgement", "Idle low point": "judgement",
    "Boost at its target": "judgement", "Charge temperature under boost": "recalled",
    "Lambda at full load under boost": "recalled",
    "Volumetric efficiency at its torque peak": "recalled", "Volumetric efficiency low down": "judgement",
    "Port gas at full load": "judgement", "Header gas at full load": "judgement",
    "Port gas, mass mean (information)": "judgement",
    "Quarter mile: time": "recalled",
    "Torque at 1500 rpm": "cited", "Torque at 1750 rpm": "cited", "Torque at 1800 rpm": "cited", "Torque at 2500 rpm": "cited",
    "Idle manifold pressure": "recalled",
    "Torque at 4000 rpm": "cited", "Torque at 6200 rpm": "cited", "Quarter mile: trap speed": "recalled", "Top speed": "judgement",
}
# builds with a part fitted to their own rows (the lead's list, 20:46): their power, torque, speed, VE and boost rows
# are judgement until the part comes from the real engine or a class default
_PORTS = "its head's port quality is a class fitted by N1 to measured torque over the target (content/builds/archetypes/generate.py PORTS)"
FITTED = {s_: _PORTS for s_ in ("big-block-74-pushrod-v8", "small-block-57-carb", "supercharged-v8-62", "pushrod-v6-38",
                                 "vr6-28", "flat-plane-v8-45", "v10-52", "v12-65", "straight-six-32-high-rev",
                                 "two-stage-cam-20", "flat-plane-v8-39-twin-turbo", "inline-five-turbo-rally")}
FITTED["kei-660-triple-turbo"] = "its boost taper was measured by N2 to meet the class's power cap"
# builds whose power and torque are SET by a stated full-load fuel curve (the lead, 04:52; D2's): those rows are no check
# of the model and are left out of the counts (basis "set"); BSFC and the rest check it
SET = {"tractor-15-inline-six": "its full-load fuel curve sets them (D2's, from the class's cited torque through the "
                                "build's own 8.34 N m a mg): SET BY THE FUEL CURVE, out of the counts"}
SET_ROWS = ("Peak power", "Peak torque", "Speed at peak power", "Speed at peak torque")
# rows a build misses for a cause measured and named, to be cured in a later window: still rows, counted, the cause in
# their source (the lead's ruling, 25 Nov: the rotary's rows above its second turbo's join, N2's measurement)
KNOWN = {"rotary-twin-13": (("Peak power", "Speed at peak power", "Peak torque", "Speed at peak torque",
                             "Boost at its target"),
                            "A KNOWN MISS above the 4500 rpm join, its cause measured (docs/measurements/calibration/"
                            "boost/rotary-pair.md, N2's): the secondary turbine starved by the transfer valve in series "
                            "with its nozzle, and the pair oversized for its match; the re-match (two HT-12s at 0.70 bar, "
                            "an even split: sources section 23) is a later window's")}

# the boosted builds whose steady rows read a transient at the old settle (N2's named cells, package-final.md "Validation
# rows that read a transient": still spooling 4 s after the step, within 0.1 % once settled for 60 s): their full-load
# rows read on a 60 s run from 30 to 60 s (N2's convention, the last 30 of 60), their dyno steps held 20 s (the lead's
# ruling, 25 Nov); the others as before
RETIME = {"boxer-four-turbo-unequal", "diesel-20-common-rail", "inline-five-turbo-rally", "kei-660-triple-turbo",
          "pickup-diesel-67-inline-six", "straight-six-twin-turbo-30", "tractor-15-inline-six", "turbo-four-20-gdi",
          "twin-turbo-hot-vee-v8-40", "v6-35-twin-turbo-pickup"}
RETIME_SETTLE = 20

# rows a ceiling the build states holds by construction (ecu.torqueMax / powerMax: docs/models/diesel.md, "What it holds
# stops counting as validation"; the lead's ruling, 25 Nov): HELD BY CALIBRATION, shown and out of the counts. Where the
# plateau begins, BSFC, boost, lambda and the gases stay rows: the ceiling does not decide them
HELD = {"diesel-20-common-rail": (("Peak torque",), "its build states ecu.torqueMax 320 N m (VW's, cited): the peak is "
                                  "held there by construction: HELD BY CALIBRATION, out of the counts")}
FITTED_ROWS = ("Peak power", "Peak torque", "Speed at peak power", "Speed at peak torque", "Boost at its target",
               "Volumetric efficiency at its torque peak", "Volumetric efficiency low down")

# a maker's published torque curve, as rows (rpm, N m low, N m high, source); the band about the published figure 5 %
# (judgement); between the plateau's ends the maker's figure is the plateau's
_GTI = ("the EA888 gen 3's 350 N m from 1500 to 4400 rpm and 162 kW at 4500 to 6200 (volkswagen-newsroom.com, the "
        "Mk7 GTI's press presentation), within 5 %% (judgement): %s")
CURVE = {
    "turbo-four-20-gdi": [
        (1500, 333, 368, _GTI % "the plateau's start"), (1800, 333, 368, _GTI % "on the plateau"),
        (2500, 333, 368, _GTI % "on the plateau"), (4000, 333, 368, _GTI % "on the plateau"),
        (6200, 237, 262, _GTI % "at rated speed, 162 kW: 250 N m")],
    # VW's 320 N m from 1750 rpm (the Mk7 Golf Estate press presentation, volkswagen-newsroom.com: the 110 kW 2.0 TDI's
    # "maximum torque of 320 Nm is available from 1,750 rpm (and up to 3,000 rpm)"; sources section 20), within 5 %
    # (judgement): where the plateau begins is the air's, not the ceiling's (the lead's ruling, 25 Nov)
    "diesel-20-common-rail": [
        (1750, 304, 336, "VW's 2.0 TDI (110 kW): \"maximum torque of 320 Nm is available from 1,750 rpm (and up to 3,000 "
                         "rpm)\" (volkswagen-newsroom.com, the Mk7 Golf Estate's press presentation), within 5 % "
                         "(judgement): the plateau's start, which the torque ceiling does not decide")],
}

# archetypes whose power and torque rows were looked up (slug: the pages), turning recalled into cited
CITED = {
    "air-cooled-flat-six-36": "en.wikipedia.org/wiki/Porsche_964 (184 kW at 6100, 310 N m at 4800)",
    "vr6-28": "adac.de autokatalog, VW Vento VR6 (128 kW at 5800, 235 N m at 4200)",
    "twin-cam-roadster-18": "auto-data.net, MX-5 NB 1.8 (103 kW at 6500, 162 N m at 4500; 107 kW at 7000, 168 N m at 5000)",
    "small-block-57-carb": "corvsport.com/1977-corvette-specifications (L48 134 kW at 4000, 366 N m at 2400; L82 157 kW "
                           "at 5200, 346 N m at 3600); corvetteactioncenter.com/specs/c3/c3power.html (1975 to 1980)",
    "pushrod-v6-38": "3800wiki.com/overview (153 kW at 5200, 312 N m at 4000); Wikipedia's GM 3800 (205 hp, 230 lb ft)",
    "big-bore-70-pushrod-v8": "corvetteactioncenter.com/specs/c6/2006/2006ls7.html (377 kW at 6300, 637 N m at 4800, "
                              "SAE J2723)",
    "four-cam-v8-50": "en.wikipedia.org/wiki/Ford_Modular_engine (343 kW at 7000, 569 N m at 4600)",
    "flat-plane-v8-45": "en.wikipedia.org/wiki/Ferrari_458 (419 kW at 9000, 540 N m at 6000)",
    "two-stage-cam-20": "en.wikipedia.org/wiki/Honda_F20C_engine (JDM 184 kW at 8300, 218 N m at 7500; US 179 kW, 210 N m)",
    "v6-35-na": "en.wikipedia.org/wiki/Honda_J_engine (J35Z2 200 kW at 6200, 336 N m at 5000; J35Y1 207 kW, 342 N m at "
                "4900); en.wikipedia.org/wiki/Toyota_GR_engine (2GR-FE 200-234 kW at 6200, 336-353 N m at 4700)",
    "v10-52": "en.wikipedia.org/wiki/Audi_R8_(Type_42) (386-412 kW at 8000, 530-540 N m at 6500); "
              "en.wikipedia.org/wiki/Lamborghini_Gallardo (LP560-4 412 kW, 540 N m)",
    "big-block-74-pushrod-v8": "carnut.com/specs/gen/vette70.html (LS5 1972 201 kW at 4000, 529 N m at 3200; LS4 1973-74 "
                               "201-205 kW at 4000-4400, 515-536 N m at 2800)",
    "economy-four-15": "en.wikipedia.org/wiki/Honda_L_engine (L15B 97 kW at 6600, 155 N m at 4600); auto-data.net Mazda2 "
                       "1.5 (85 kW at 6000, 148 N m at 4000); en.wikipedia.org/wiki/Toyota_NR_engine (2NR-FKE 80 kW, 136 N m)",
    "hot-hatch-16-16v": "en.wikipedia.org/wiki/Honda_B_engine (B16A 110-118 kW at 7600, 144-150 N m at 7000-7100); "
                        "en.wikipedia.org/wiki/Toyota_A_engine (4A-GE 91-103 kW at 6600-7200, 142-147 N m at 5200-6000)",
    "straight-six-32-high-rev": "en.wikipedia.org/wiki/BMW_S54 (252 kW at 7900, 365 N m at 4900)",
    "v12-65": "en.wikipedia.org/wiki/Ferrari_F140_engine (F12 544 kW at 8250, 690 N m at 6000; 812 588 kW at 8500, 718 "
              "N m at 6750); en.wikipedia.org/wiki/Lamborghini_Aventador (515-566 kW, 690-720 N m at 5500-6750)",
    "turbo-four-20-gdi": "volkswagen-newsroom.com, the new Golf GTI's international press presentation (162 kW at "
                         "4500-6200 rpm, 350 N m at 1500-4400; the Performance 169 kW, 350 N m at 1500-4600); Audi SSP "
                         "920243 (the EA888 gen 3: 82.5 x 92.8 mm, 9.6:1)",
    "rotary-twin-na-side-port": "newsroom.mazda.com 2003 release (184 kW at 8500, 216 N m at 5500); "
                                "en.wikipedia.org/wiki/Mazda_RX-8 (US 173 kW at 8250, 216 N m at 5500)",
}

# idle manifold pressure, warm (the lead, 01:46: the total trapped mass at idle; targets, nothing tuned to them), kPa
# absolute (low, high, basis, source); docs/measurements/2026-11-25-0032-judgement-sources.md section 10
_MOTOR = ("MOTOR magazine, Vacuum Gauge & Transducer Testing (July 2006): \"On a normal engine, normal idle vacuum "
          "should be a steady reading between 17 and 21 in./Hg\" (warm), 30 to 44 kPa absolute at 101.3 kPa; the "
          "class figure, the engine's own not found")
# (not the 3.2: a throttle body a runner, no plenum, as the S54's, which has no manifold sensor: its manifold reads a
# runner's pulsing mean, not what the class figure means; not applicable, the lead 01:55)
IDLE_MAP = {
    "economy-four-15": (20, 34, "cited", "Honda's L15 ECM/PCM data table: \"At idle speed: about 20-34 kPa (150-260 "
                        "mmHg, 6-10 in.Hg)\", warm, in neutral or park, A/C and accessories off, 750 rpm (the L15A1/A6 "
                        "page): the same class's engine"),
    "hot-hatch-16-16v": (43, 53, "recalled", "the 4A-GE's service specification as quoted in an aeu86.org forum post from "
                         "Toyota's 1987 repair manual, p. A6 (the manual not read): \"53.5 kPa (abs) = 15.75\" Hg @ 800 "
                         "rpm idle\", one reading: 15.75 inHg is 400 mmHg (53.3 kPa) of vacuum, about 48 kPa absolute "
                         "(the post's \"abs\" a mislabel, the lead 01:55); 48 within 5 kPa (judgement); the post does not "
                         "say whether it is a minimum, and it MAY BE A MINIMUM vacuum (then 48 is an upper bound)"),
    "big-bore-70-pushrod-v8": (30, 44, "recalled", _MOTOR + "; a stock LS7 at 38 kPa (an HP Tuners forum post) inside it"),
    "v10-52": (30, 44, "recalled", _MOTOR + "; no figure of the 5.2 V10's own found"),
    "v12-65": (30, 44, "recalled", _MOTOR + "; no figure of a production NA V12's own found"),
}

# slug: published peak power (kW range, rpm range), torque (N m range, rpm range), warm idle (rpm range),
# boost check speed (rpm of a p-full-load scenario), the source's firmness, and top speed if the kind's is published
# unrestricted (km/h range; None: worked out from power and drag).
T = {
    "straight-six-twin-turbo-30": ((250, 340), (5000, 6800), (450, 600), (1500, 5200), (600, 750), 5000, "makers' figures, firm"),
    "flat-plane-v8-45": ((400, 445), (8000, 9000), (520, 560), (5500, 6500), (900, 1100), 0, "the maker's figures, firm"),
    "air-cooled-flat-six-36": ((180, 195), (5800, 6300), (300, 320), (4500, 5000), (800, 950), 0, "makers' figures, firm"),
    # (moved from 240-310 kW at 4400-5600 and 600-690 N m at 3000-4000, gross-like: the 454's SAE net ratings 1972-74, looked up)
    "big-block-74-pushrod-v8": ((195, 210), (4000, 4400), (510, 540), (2800, 3200), (550, 750), 0, "SAE net ratings, looked up"),
    "rotary-twin-13": ((190, 210), (6300, 7000), (290, 315), (4500, 5500), (750, 900), 5000, "makers' figures, firm to 10 % (variants differ)"),
    "rotary-twin-na-side-port": ((165, 190), (8000, 8800), (205, 220), (5000, 6000), (750, 900), 0, "makers' figures by market and year, from memory, firm to 5 %"),
    # (moved from 140-165 N m at 4500-6000: the B16A's 144-150 at 7000-7100 and the 4A-GE's 142-147 at 5200-6000, looked up)
    "hot-hatch-16-16v": ((85, 120), (6500, 7600), (140, 155), (5000, 7100), (800, 950), 0, "makers' figures of the kind, looked up"),
    "supercharged-v8-62": ((395, 435), (5800, 6300), (720, 780), (3500, 4200), (650, 750), 5000, "makers' figures, firm"),
    # (the L48 and L82's SAE net ratings, looked up 24 Nov: the old 140-210 kW and 380-470 N m were above both)
    "small-block-57-carb": ((120, 160), (4000, 5200), (340, 385), (2400, 3600), (600, 800), 0, "SAE net ratings, looked up"),
    "v10-52": ((385, 475), (8000, 8700), (520, 570), (6000, 7000), (850, 1000), 0, "makers' figures, firm"),
    # (moved from 6000-7000 rpm at peak torque: the Aventador LP700-4's 690 N m at 5500, looked up)
    "v12-65": ((500, 590), (8250, 9000), (680, 730), (5500, 7000), (850, 1000), 0, "makers' figures, looked up"),
    "inline-five-turbo-rally": ((200, 265), (5500, 7000), (380, 520), (3500, 5500), (900, 1150), 5000, "rally engines of the kind, loose (homologation rules and restrictors vary)"),
    "boxer-four-turbo-unequal": ((210, 235), (5800, 6400), (380, 410), (3600, 4400), (700, 900), 5000, "makers' figures, firm"),
    # (moved from 165-185 kW and 200-215 N m: the F20C's JDM 184 kW / 218 N m and US 179 kW / 210 N m, looked up)
    "two-stage-cam-20": ((170, 185), (8000, 8600), (205, 220), (7000, 8000), (850, 950), 0, "makers' figures, looked up"),
    "kei-660-triple-turbo": ((44, 50), (6000, 7000), (100, 110), (3000, 4000), (750, 950), 5000, "makers' figures, firm (the class's 47 kW cap)"),
    "diesel-20-common-rail": ((100, 130), (3500, 4200), (320, 380), (1700, 3000), (750, 900), 2000, "makers' figures, firm"),
    "tractor-15-inline-six": ((300, 450), (1600, 2000), (2000, 2780), (900, 1300), (600, 700), 1200, "the maker's ratings, firm (the range covers its ratings)"),
    "three-cylinder-10-turbo": ((74, 105), (5000, 6000), (170, 210), (1500, 4500), (700, 900), 3000, "makers' figures, firm"),
    # (moved from 140-156 N m: the 2NR-FKE's 136, looked up)
    "economy-four-15": ((80, 97), (6000, 6600), (135, 156), (4000, 4800), (650, 800), 0, "makers' figures, looked up"),
    # (the EA888 gen 3 of the Mk7 GTI, looked up (the lead, 22:49): moved from 155-230 kW at 5000-6500 and 350-400 N m at
    # 1600-4500; the band about the maker's figure judgement: 5 % on torque, the two versions' power)
    "turbo-four-20-gdi": ((158, 172), (4500, 6200), (335, 370), (1500, 4400), (650, 850), 5000, "the maker's figures, looked up"),
    "v6-35-na": ((195, 225), (6000, 6700), (335, 365), (4500, 5000), (600, 750), 0, "makers' figures, firm"),
    "v6-35-twin-turbo-pickup": ((275, 305), (5000, 6000), (630, 700), (3000, 4000), (550, 700), 4000, "makers' figures, firm"),
    "twin-cam-roadster-18": ((95, 112), (6300, 7000), (155, 172), (4500, 5500), (750, 900), 0, "makers' figures, firm"),
    "pickup-diesel-67-inline-six": ((255, 315), (2700, 3100), (1000, 1470), (1500, 1900), (650, 800), 2000, "makers' figures, firm (the range covers its ratings)"),
    "twin-turbo-hot-vee-v8-40": ((350, 470), (5500, 6600), (700, 850), (2000, 4500), (550, 700), 5000, "makers' figures, firm"),
    "vr6-28": ((125, 132), (5600, 6000), (230, 240), (4000, 4400), (700, 800), 0, "makers' figures from memory, firm to 5 %"),
    "straight-six-32-high-rev": ((245, 260), (7600, 8000), (355, 375), (4600, 5200), (800, 950), 0, "makers' figures from memory, firm to 5 %"),
    "four-cam-v8-50": ((300, 345), (6500, 7200), (520, 575), (4200, 4800), (600, 750), 0, "makers' figures, firm"),
    "big-bore-70-pushrod-v8": ((370, 385), (6200, 6400), (630, 645), (4600, 5000), (650, 800), 0, "the maker's figures, firm"),
    "pushrod-v6-38": ((140, 160), (5000, 5400), (300, 330), (3800, 4200), (600, 750), 0, "makers' figures of the 1990s, firm"),
    "flat-plane-v8-39-twin-turbo": ((450, 535), (7500, 8000), (760, 805), (3000, 5750), (800, 1000), 5000, "makers' figures, firm"),
}
# table L's content (the lead, 02:12), with the builds' CAM_RULE: the hot hatch of the 4A-GE kind whole, its bands the
# 4A-GE's (moved from 85-120 kW at 6500-7600 and 140-155 N m at 5000-7100, which spanned the B16A too)
CAM_RULE = os.environ.get("TEE_CAM_RULE") != "0"   # on for the naturally aspirated with table L (the lead, 04:07)
if CAM_RULE:
    T["hot-hatch-16-16v"] = ((91, 103), (6600, 7200), (142, 147), (5200, 6000), (800, 950), 0,
                             "the 4A-GE's makers' figures, looked up")
    CITED["hot-hatch-16-16v"] = ("en.wikipedia.org/wiki/Toyota_A_engine (4A-GE 91-103 kW at 6600-7200, 142-147 N m at "
                                 "5200-6000)")

HEYWOOD = "Heywood, Internal Combustion Engine Fundamentals"


def full_load_speed(rpm, redline):
    """The nearest p-full-load scenario's speed at or under 0.9 of the redline."""
    s = [r for r in (1200, 2000, 3000, 4000, 5000, 6000, 7500) if r <= 0.9 * redline]
    return min(s, key=lambda r: abs(r - rpm))


def top_speed(car, kw, redline):
    """km/h: where the wheel power (0.85 of the crank's) meets drag and rolling resistance on level ground at
    1.225 kg/m^3, or the redline in top gear if that comes first."""
    p = 0.85 * kw * 1000.0
    v = 1.0
    for _ in range(200):
        need = 0.5 * 1.225 * car["cdA"] * v ** 3 + 0.012 * car["mass"] * 9.81 * v
        v *= (p / need) ** (1.0 / 3.0)
    top_ratio = min(car["ratio"]) if car["gearbox"] != "CVT" else car.get("cvtHigh", 0.42)
    v_red = redline / (top_ratio * car["finalDrive"]) * car["tyreRadius"]
    return min(v, v_red) * 3.6, v < v_red


def targets(slug, b):
    (plo, phi), (prlo, prhi), (tlo, thi), (trlo, trhi), (ilo, ihi), boost_at, firm = T[slug]
    s = b["spec"]
    e, car = s["engine"], s["car"]
    litres = e["displacement"] * 1000.0
    red = e["redlineSpeed"]
    red_rpm = red / RPM
    diesel = s["fuel"]["kind"] == "Diesel"
    boosted = s["boost"]["kind"] != "Natural"
    auto = car["gearbox"] in ("Automatic", "CVT")
    dyno = {"from": round(max(ilo * RPM, 60.0), 1), "to": round(red, 1)}
    if boosted:
        # on the full engine dyno, held at each speed 5 s before 1 s is measured: the predicted curve's short settling
        # leaves a boosted engine's slow states (knock retard, the intercooler's core, the shaft) unsettled and reads
        # some 10 % high (N2's measurement on the 2.0 L turbo four)
        lo = max(1500.0, ilo * 1.5)
        n = 9
        dyno = {"steps": [round((lo + (0.98 * red / RPM - lo) * i / (n - 1)) * RPM, 1) for i in range(n)],
                "settle": RETIME_SETTLE if slug in RETIME else 5, "measure": 1}
    src = "published figures of its kind (%s): %s" % (firm, b["notes"].split("(")[1].split(")")[0] if "(" in b["notes"] else "")
    out = []

    def add(**k):
        # what the target is measured against (the lead, 20:23): cited (a page, a figure, a standard or a maker's datum
        # looked up, named in the source), recalled (a maker's figure or a textbook range written from memory, believed
        # right) or judgement (a band set because it seemed reasonable: it catches a wild value; nobody should move
        # the model toward it)
        name = k["name"]
        if name in ("Peak power", "Peak torque", "Speed at peak power", "Speed at peak torque") and slug in CITED:
            k["basis"] = "cited"
            k["source"] = k["source"] + "; looked up: " + CITED[slug]
        else:
            k["basis"] = "cited" if name.startswith("Invariant") else BASIS[name]
        # a row its build was fitted to is judgement, whatever its source (the lead, 20:52: so the cited count is not
        # flattered while the part is replaced)
        if slug in SET and name in SET_ROWS:
            k["basis"] = "set"
            k["source"] = k["source"] + "; " + SET[slug]
        if slug in KNOWN and name in KNOWN[slug][0]:
            k["source"] = k["source"] + "; " + KNOWN[slug][1]
        if slug in HELD and name in HELD[slug][0]:
            k["basis"] = "held"
            k["source"] = k["source"] + "; " + HELD[slug][1]
        if name == "Idle manifold pressure":
            k["basis"] = IDLE_MAP[slug][2]
        if slug in FITTED and name in FITTED_ROWS:
            k["basis"] = "judgement"
            k["source"] = k["source"] + "; JUDGEMENT here: " + FITTED[slug]
        out.append(k)

    # ---- the invariants: hard --------------------------------------------------------------------------------
    add(name="Invariant: starts warm and runs", scenario=SC + "p-idle-warm.json", tel="engine.state", measure="min",
        during=[6, 14], range=[2, 2], source="a warm engine started on its key runs and keeps running (engine.state 2)")
    add(name="Invariant: within full scale", scenario=SC + "p-free-rev.json", audio="peak", range=[0, 1],
        source="02-contracts.md, section 1: what process writes is within +-1")
    # ---- the sound's level: floors beside the cap (the lead, 20:11: with L's listener on, a first launch played 23 to
    # 30 dB under the reference mix, near silent, and no row caught it) --------------------------------------------
    add(name="Sound: a free rev is loud enough", scenario=SC + "p-free-rev.json", audio="peak", range=[0.2, 1],
        source="the lead's ruling (20:11): at a build's defaults a free rev peaks at 0.3 to 0.6 of full scale; under "
               "0.2 is too quiet to hear the work (L's listener at 23 to 30 dB under the reference mix: L's finding)")
    add(name="Sound: the idle is audible", scenario=SC + "p-idle-warm.json", audio="rms", during=[8, 14],
        range=[-40, 0], unit="dB",
        source="the lead's ruling (20:11): the idle plainly audible: its level over 8 to 14 s at least -40 dB of full "
               "scale (judgement: a quiet room's background against a full-scale peak)")
    gov = s["ecu"].get("governor") == "Electronic"
    if gov:
        # a governed diesel: its limit is the governor's high idle (rated x 1.06), read absolutely: tel.engine.redline
        # is floored at idle + 1500 rpm by the ECU, above a slow truck's high idle (D2's finding, for H and D)
        hi = e["redlineSpeed"]
        add(name="Invariant: holds its limiter", scenario=SC + "p-free-rev.json", tel="engine.speed", measure="max",
            during=[6, 9], range=[round(0.97 * hi, 1), round(1.03 * hi, 1)], unit="rad/s",
            source="a diesel's governor holds its high idle, the rated speed plus its 6 %% droop (%d rpm), within a few "
                   "percent" % round(hi / RPM))
    else:
        add(name="Invariant: holds its limiter", scenario=SC + "p-free-rev.json", tel="engine.speed", measure="max",
            during=[6, 9], relativeTo="engine.redline", range=[0.97, 1.03], unit="of the redline",
            source="a rev limiter holds the speed within a few percent of where it cuts (some 100 to 200 rpm at 6500)")
    pull = SC + ("p-pull-auto.json" if auto else "p-pull-eighteen.json" if car["gearbox"] == "Eighteen-speed"
                 else "p-pull-paddles.json")
    add(name="Invariant: pulls away", scenario=pull, tel="car.speed", measure="max", during=[3, 13],
        range=[1.5 if slug.startswith("tractor") else 8, 120], unit="m/s",
        source="started, in gear and on the pedal, the car moves (a 36 t tractor in third of eighteen at least 1.5 m/s)")
    add(name="Invariant: stops at the key", scenario=pull, tel="engine.state", measure="last", range=[0, 0],
        source="the key off stops the engine (engine.state 0 by the run's end)")
    add(name="Starts cold", scenario=SC + "p-cold-start.json", tel="engine.state", measure="min", during=[6, 11.5],
        range=[2, 2], source="a sound engine starts from the air's temperature within some four seconds of cranking "
                             "and keeps running (a rule of thumb; makers' cold-start specifications are 2 to 5 s)")
    # a hot start (the lead, 22:51: the V12 took over 1.5 s to start in 318 K air in card I's run): the engine warm in
    # 318 K air, started within the cold row's time and idling in its band
    add(name="Starts hot", scenario=SC + "p-hot-start.json", tel="engine.state", measure="min", during=[6, 11.5],
        range=[2, 2], source="a warm engine in 318 K air (a hot day's under-bonnet) starts within some four seconds of "
                             "cranking and keeps running (judgement, as the cold row)")
    _asked = s["ecu"]["idle"]["speed"] / RPM
    add(name="Hot idle", scenario=SC + "p-hot-start.json", tel="engine.speed", measure="mean", during=[8, 11.5],
        range=[round((_asked - 100) * RPM, 1), round((_asked + 100) * RPM, 1)], unit="rad/s",
        source="the warm engine in 318 K air idles within 100 rpm of the %d rpm its ECU asks (judgement)" % round(_asked))

    # ---- the dyno ---------------------------------------------------------------------------------------------
    add(name="Peak power", dyno=dyno, measure="maxPower", range=[plo * 1000, phi * 1000], unit="W",
        source="%s; %.0f to %.0f kW a litre" % (src, plo / litres, phi / litres))
    add(name="Speed at peak power", dyno=dyno, measure="speedAtMaxPower",
        range=[round(prlo * RPM, 1), round(min(prhi, red_rpm) * RPM, 1)], unit="rad/s",
        source="%s: %d to %d rpm" % (src, prlo, prhi))
    k = 4 * math.pi / e["displacement"] / 1e5
    add(name="Peak torque", dyno=dyno, measure="maxTorque", range=[tlo, thi], unit="N m",
        source="%s; BMEP %.1f to %.1f bar (a naturally aspirated petrol engine 10 to 13 bar, turbocharged 18 to 25, a "
               "modern truck diesel 20 to 25: %s, ch. 2)" % (src, tlo * k, thi * k, HEYWOOD))
    add(name="Speed at peak torque", dyno=dyno, measure="speedAtMaxTorque",
        range=[round(trlo * RPM, 1), round(trhi * RPM, 1)], unit="rad/s",
        source="%s: %d to %d rpm (a turbocharged engine's plateau: its whole span)" % (src, trlo, trhi))

    # ---- idle -------------------------------------------------------------------------------------------------
    add(name="Warm idle", scenario=SC + "p-idle-warm.json", tel="engine.speed", measure="mean", during=[8, 14],
        range=[round(ilo * RPM, 1), round(ihi * RPM, 1)], unit="rad/s",
        source="makers' service specifications of the kind: %d to %d rpm warm in neutral (a rule of thumb, firm to "
               "100 rpm); the ECU is asked for %d" % (ilo, ihi, round(s["ecu"]["idle"]["speed"] / RPM)))
    if slug in IDLE_MAP:
        lo, hi, _, msrc = IDLE_MAP[slug]
        add(name="Idle manifold pressure", scenario=SC + "p-idle-warm.json", tel="intake.manifold", measure="mean",
            during=[8, 14], range=[lo * 1000.0, hi * 1000.0], unit="Pa", source=msrc)
    # its spread as two targets, its highest and its lowest point about the speed asked, so a hunt cannot pass on its
    # mean (the lead, 19:50: the 7.0's idle between 497 and 929 rpm passed its mean and its old max of the range's top
    # plus 150 rpm)
    asked = s["ecu"]["idle"]["speed"] / RPM
    add(name="Idle steadiness", scenario=SC + "p-idle-warm.json", tel="engine.speed", measure="max", during=[6, 14],
        range=[round(asked * RPM, 1), round((asked + 100) * RPM, 1)], unit="rad/s",
        source="a warm idle's speed wanders some 20 to 50 rpm (a rule of thumb): its highest point from 6 to 14 s more "
               "than 100 rpm over the %d rpm asked is a hunt" % round(asked))
    add(name="Idle low point", scenario=SC + "p-idle-warm.json", tel="engine.speed", measure="min", during=[6, 14],
        range=[round((asked - 100) * RPM, 1), round(asked * RPM, 1)], unit="rad/s",
        source="a warm idle's speed wanders some 20 to 50 rpm (a rule of thumb): its lowest point from 6 to 14 s more "
               "than 100 rpm under the %d rpm asked is a hunt (or a stall coming)" % round(asked))

    # ---- full load: VE, boost, the port's gas ----------------------------------------------------------------
    fl = full_load_speed(trhi if not boosted else max(boost_at, 1200), red_rpm)
    fl_sc, fl_during = ("p-full-load-%d-long.json", [30, 60]) if slug in RETIME else ("p-full-load-%d.json", [8, 10])
    if boosted:
        fl = boost_at if boost_at and boost_at <= 0.9 * red_rpm else fl
        tgt = s["boost"]["target"]
        add(name="Boost at its target", scenario=SC + fl_sc % fl, tel="boost.pressure", measure="mean",
            during=fl_during, range=[tgt - 15000, tgt + 15000], unit="Pa over the air",
            source="the build's own target (%.2f bar); a wastegate or vane loop holds a steady full load within some "
                   "0.1 bar of its target once spooled (makers' boost-control tolerances; judgement)" % (tgt / 1e5))
        add(name="Charge temperature under boost", scenario=SC + fl_sc % fl, tel="boost.throttleIn",
            measure="mean", during=fl_during, range=[293, 340], unit="K",
            source="an air-to-air intercooler of 0.6 to 0.8 effectiveness leaves the charge some 10 to 50 K over the "
                   "outside air at full boost (makers' and tuners' intake-air-temperature logs; judgement): reported so "
                   "the over-delivery at a given boost can be judged (the lead's ask)")
        add(name="Lambda at full load under boost", scenario=SC + fl_sc % fl, tel="ecu.lambda",
            measure="mean", during=fl_during, range=[1.2, 2.0] if diesel else [0.76, 0.88], unit="ratio",
            source=("a turbo diesel at full load runs lambda 1.2 to 2 (smoke-limited; %s, ch. 10)" if diesel else
                    "a boosted petrol engine at full load is enriched to lambda 0.76 to 0.88 to cool the charge and the "
                    "turbine (factory calibrations; %s, ch. 7)") % HEYWOOD)
    else:
        add(name="Volumetric efficiency at its torque peak", scenario=SC + fl_sc % fl, tel="engine.veAir",
            measure="mean", during=fl_during, range=[0.82, 1.02], unit="fraction",
            source="a naturally aspirated four-stroke's VE at full load near its torque peak is 0.85 to 1.0 (%s, ch. "
                   "6, fig. 6-9 and the text); measured at %d rpm" % (HEYWOOD, fl))
        lo_rpm = 2000 if red_rpm > 4000 else 1200
        add(name="Volumetric efficiency low down", scenario=SC + "p-full-load-%d.json" % lo_rpm, tel="engine.veAir",
            measure="mean", during=[8, 10], range=[0.70, 0.95], unit="fraction",
            source="VE's shape: below the torque peak it falls away, the more so the hotter the cam (%s, ch. 6); "
                   "measured at %d rpm" % (HEYWOOD, lo_rpm))
    tport = [850, 1100] if diesel else [1100, 1300]
    add(name="Port gas at full load", scenario=SC + fl_sc % fl, tel="exhaust.portThermocouple",
        measure="mean", during=fl_during, range=tport, unit="K",
        source=("what a thermocouple at a diesel's exhaust port reads at full load (a time mean, below the gas's mass "
                "mean: the blowdown is most of the mass and little of the time): some 850 to 1100 K (judgement; %s, ch. "
                "14); tel.exhaust.portThermocouple, card G's mixing cup" if diesel else
                "what a thermocouple at the exhaust port reads at full load (a time mean, below the gas's mass mean): "
                "some 1100 to 1300 K for a spark-ignition engine (%s, ch. 14; card C's target on default.json); "
                "tel.exhaust.portThermocouple, card G's mixing cup") % HEYWOOD)
    thead = [700, 950] if diesel else [900, 1100]
    add(name="Header gas at full load", scenario=SC + fl_sc % fl, tel="exhaust.headerThermocouple",
        measure="mean", during=fl_during, range=thead, unit="K",
        source=("a thermocouple a little way down a diesel's manifold reads some 700 to 950 K at full load (makers' "
                "pre-turbine limits near 1000 K; %s, ch. 14); tel.exhaust.headerThermocouple" if diesel else
                "a thermocouple a little way down a primary reads 900 to 1100 K at full load on a petrol engine "
                "(JUDGEMENT from memory of makers' exhaust-gas-temperature data, not a cited full-load figure: under "
                "review, the lead's question of 20:17; %s, ch. 14); tel.exhaust.headerThermocouple") % HEYWOOD)
    add(name="Port gas, mass mean (information)", scenario=SC + fl_sc % fl,
        tel="exhaust.portTemperature", measure="mean", during=fl_during, range=[0, 3000], unit="K",
        source="the gas's mass (enthalpy) mean at the port, kept for information: above a thermocouple's reading, not "
               "a target (card G, f703892)")

    # ---- the car ----------------------------------------------------------------------------------------------
    pmid = 0.5 * (plo + phi)
    qm = "p-quarter-mile-own.json" if auto else "p-quarter-mile.json"
    if os.path.exists(os.path.join(ROOT, "content", "validation", "scenarios", "qm", slug + ".json")):
        # the car's own gearbox and card I's launch, its upshifts placed by quarter_mile.py
        qm = "qm/" + slug + ".json"
    if not slug.startswith("tractor"):
        lb, hp = car["mass"] * 2.20462, pmid * 1.341
        et, trap = 5.825 * (lb / hp) ** (1 / 3), 234 * (hp / lb) ** (1 / 3) * 1.609344 / 3.6
        add(name="Quarter mile: time", scenario=SC + qm, tel="car.t1320", measure="last",
            range=[round(et * 0.95, 2), round(et * 1.15, 2)], unit="s",
            source="Hale's rule, ET = 5.825 (lb/hp)^(1/3) s, with the kind's published power (%.0f kW) and the build's "
                   "mass: %.2f s; 5 %% quicker to 15 %% slower for the build's own tyres and launch (card I's "
                   "test holds drag radials to 8 %%)" % (pmid, et))
        add(name="Quarter mile: trap speed", scenario=SC + qm, tel="car.v1320", measure="last",
            range=[round(trap * 0.92, 1), round(trap * 1.06, 1)], unit="m/s",
            source="Hale's rule, trap = 234 (hp/lb)^(1/3) mph: %.1f m/s (%.0f km/h), within 8 %% under to 6 %% over"
                   % (trap, trap * 3.6))
    if slug.startswith("tractor"):
        return out     # a 36 t tractor needs minutes to reach its cruising speed: no top-speed run is that long
    vmax, aero = top_speed(car, pmid, red)
    add(name="Top speed", scenario=SC + ("p-top-speed-own.json" if auto else "p-top-speed.json"), tel="car.speed", measure="max",
        range=[round(vmax / 3.6 * 0.93, 1), round(vmax / 3.6 * 1.07, 1)], unit="m/s",
        source=("where 0.85 of the kind's published power (%.0f kW) meets the build's drag area and 1.2 %% rolling "
                "resistance at 1.225 kg/m^3: %.0f km/h, within 7 %% (a power balance, not a test)" % (pmid, vmax))
        if aero else ("the redline in top gear with the build's gearing: %.0f km/h, within 7 %% (geared, not "
                      "drag-limited); no speed governor is simulated" % vmax))
    # the published torque curve where a maker gives its plateau (the lead, 22:49: the first-launch engine's low end):
    # rows on the full engine dyno at each speed, cited
    for rpm_, lo_, hi_, why in CURVE.get(slug, ()):
        add(name="Torque at %d rpm" % rpm_, dyno={"steps": [round(rpm_ * RPM, 1)], "settle": RETIME_SETTLE if slug in RETIME else 5, "measure": 1},
            measure="torqueAt", at=round(rpm_ * RPM, 1), range=[lo_, hi_], unit="N m", source=why)
    return out


def lines_note(b):
    """The spectral lines the firing order predicts, for the description (no audio measure of a line exists yet)."""
    e = b["spec"]["engine"]
    n = e["cylinders"]
    idle = b["spec"]["ecu"]["idle"]["speed"] / RPM
    red = e["redlineSpeed"] / RPM
    if b["spec"]["engine"]["layout"] == "Rotary":
        f_idle, f_red = idle / 60 * e.get("rotors", 2), red / 60 * e.get("rotors", 2)
        what = "a face fires each eccentric-shaft turn on each rotor"
    else:
        f_idle, f_red = idle / 60 * n / 2, red / 60 * n / 2
        what = "the firing frequency, rpm/60 x %d/2" % n
    extra = ""
    if b["spec"]["engine"]["layout"] == "V8 crossplane":
        extra = "; each bank fires unevenly, so half-order lines (%.1f Hz at idle) stand out from a single bank's pipe" \
                % (idle / 60 * 2)
    if b["spec"]["engine"]["layout"] == "V8 flat-plane":
        extra = "; each bank fires evenly (four cylinders each), so the firing line dominates and second order shakes"
    return ("Predicted strongest lines (not measured: the validation format has no line measure yet): %s, %.1f Hz at "
            "the %d rpm idle and %.0f Hz at the %d rpm limiter%s." % (what, f_idle, idle, f_red, red, extra))


def main():
    os.makedirs(HERE, exist_ok=True)
    for slug in sorted(T):
        with open(os.path.join(BUILDS, slug + ".json")) as f:
            b = json.load(f)
        doc = {"format": "tee-validation", "version": 1, "build": "builds/archetypes/" + slug + ".json",
               "description": "Card P's targets for \"%s\": the invariants are hard; the rest are reported, never "
                              "failed: their misses are the simulator's to-do list. %s" % (b["name"], lines_note(b)),
               "targets": targets(slug, b)}
        with open(os.path.join(HERE, slug + ".json"), "w") as f:
            json.dump(doc, f, indent=2)
            f.write("\n")
    print("%d validations" % len(T))


if __name__ == "__main__":
    main()
