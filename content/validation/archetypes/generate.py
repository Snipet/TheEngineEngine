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

# slug: published peak power (kW range, rpm range), torque (N m range, rpm range), warm idle (rpm range),
# boost check speed (rpm of a p-full-load scenario), the source's firmness, and top speed if the kind's is published
# unrestricted (km/h range; None: worked out from power and drag).
T = {
    "straight-six-twin-turbo-30": ((250, 340), (5000, 6800), (450, 600), (1500, 5200), (600, 750), 5000, "makers' figures, firm"),
    "flat-plane-v8-45": ((400, 445), (8000, 9000), (520, 560), (5500, 6500), (900, 1100), 0, "the maker's figures, firm"),
    "air-cooled-flat-six-36": ((180, 195), (5800, 6300), (300, 320), (4500, 5000), (800, 950), 0, "makers' figures, firm"),
    "big-block-74-pushrod-v8": ((240, 310), (4400, 5600), (600, 690), (3000, 4000), (550, 750), 0, "net ratings of the era, loose (gross and net ratings differ by 20 %)"),
    "rotary-twin-13": ((190, 210), (6300, 7000), (290, 315), (4500, 5500), (750, 900), 5000, "makers' figures, firm to 10 % (variants differ)"),
    "hot-hatch-16-16v": ((85, 120), (6500, 7600), (140, 165), (4500, 6000), (800, 950), 0, "makers' figures of the kind, loose (the kind's spread)"),
    "supercharged-v8-62": ((395, 435), (5800, 6300), (720, 780), (3500, 4200), (650, 750), 5000, "makers' figures, firm"),
    "small-block-57-carb": ((140, 210), (4000, 5200), (380, 470), (2400, 3600), (600, 800), 0, "net ratings of the era, loose"),
    "v10-52": ((385, 475), (8000, 8700), (520, 570), (6000, 7000), (850, 1000), 0, "makers' figures, firm"),
    "v12-65": ((500, 590), (8250, 9000), (680, 730), (6000, 7000), (850, 1000), 0, "makers' figures, firm"),
    "inline-five-turbo-rally": ((200, 265), (5500, 7000), (380, 520), (3500, 5500), (900, 1150), 5000, "rally engines of the kind, loose (homologation rules and restrictors vary)"),
    "boxer-four-turbo-unequal": ((210, 235), (5800, 6400), (380, 410), (3600, 4400), (700, 900), 5000, "makers' figures, firm"),
    "two-stage-cam-20": ((165, 185), (8000, 8600), (200, 215), (7000, 8000), (850, 950), 0, "makers' figures from memory, firm to 5 %"),
    "kei-660-triple-turbo": ((44, 50), (6000, 7000), (100, 110), (3000, 4000), (750, 950), 5000, "makers' figures, firm (the class's 47 kW cap)"),
    "diesel-20-common-rail": ((100, 130), (3500, 4200), (320, 380), (1700, 3000), (750, 900), 2000, "makers' figures, firm"),
    "tractor-15-inline-six": ((300, 450), (1600, 2000), (2000, 2780), (900, 1300), (600, 700), 1200, "the maker's ratings, firm (the range covers its ratings)"),
    "three-cylinder-10-turbo": ((74, 105), (5000, 6000), (170, 210), (1500, 4500), (700, 900), 3000, "makers' figures, firm"),
    "economy-four-15": ((80, 97), (6000, 6600), (140, 156), (4000, 4800), (650, 800), 0, "makers' figures, firm"),
    "turbo-four-20-gdi": ((155, 230), (5000, 6500), (350, 400), (1600, 4500), (650, 850), 5000, "makers' figures, firm"),
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
    src = "published figures of its kind (%s): %s" % (firm, b["notes"].split("(")[1].split(")")[0] if "(" in b["notes"] else "")
    out = []

    def add(**k):
        out.append(k)

    # ---- the invariants: hard --------------------------------------------------------------------------------
    add(name="Invariant: starts warm and runs", scenario=SC + "p-idle-warm.json", tel="engine.state", measure="min",
        during=[6, 14], range=[2, 2], source="a warm engine started on its key runs and keeps running (engine.state 2)")
    add(name="Invariant: within full scale", scenario=SC + "p-free-rev.json", audio="peak", range=[0, 1],
        source="02-contracts.md, section 1: what process writes is within +-1")
    add(name="Invariant: holds its limiter", scenario=SC + "p-free-rev.json", tel="engine.speed", measure="max",
        during=[6, 9], relativeTo="engine.redline", range=[0.97, 1.03], unit="of the redline",
        source="a rev limiter holds the speed within a few percent of where it cuts (some 100 to 200 rpm at 6500)")
    pull = SC + ("p-pull-auto.json" if auto else "p-pull-paddles.json")
    add(name="Invariant: pulls away", scenario=pull, tel="car.speed", measure="max", during=[3, 13],
        range=[3 if slug.startswith("tractor") else 8, 120], unit="m/s",
        source="started, in gear and on the pedal, the car moves (a 36 t tractor at least 3 m/s in ten seconds)")
    add(name="Invariant: stops at the key", scenario=pull, tel="engine.state", measure="last", range=[0, 0],
        source="the key off stops the engine (engine.state 0 by the run's end)")
    add(name="Starts cold", scenario=SC + "p-cold-start.json", tel="engine.state", measure="min", during=[6, 11.5],
        range=[2, 2], source="a sound engine starts from the air's temperature within some four seconds of cranking "
                             "and keeps running (a rule of thumb; makers' cold-start specifications are 2 to 5 s)")

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
    add(name="Idle steadiness", scenario=SC + "p-idle-warm.json", tel="engine.speed", measure="max", during=[8, 14],
        range=[round(ilo * RPM, 1), round((ihi + 150) * RPM, 1)], unit="rad/s",
        source="a warm idle's speed wanders some 20 to 50 rpm (a rule of thumb); a lope of more than 150 rpm over the "
               "range's top is a fault")

    # ---- full load: VE, boost, the port's gas ----------------------------------------------------------------
    fl = full_load_speed(trhi if not boosted else max(boost_at, 1200), red_rpm)
    if boosted:
        fl = boost_at if boost_at and boost_at <= 0.9 * red_rpm else fl
        tgt = s["boost"]["target"]
        add(name="Boost at its target", scenario=SC + "p-full-load-%d.json" % fl, tel="boost.pressure", measure="mean",
            during=[8, 10], range=[tgt - 15000, tgt + 15000], unit="Pa over the air",
            source="the build's own target (%.2f bar); a wastegate or vane loop holds a steady full load within some "
                   "0.1 bar of its target once spooled (makers' boost-control tolerances; judgement)" % (tgt / 1e5))
        add(name="Charge temperature under boost", scenario=SC + "p-full-load-%d.json" % fl, tel="boost.throttleIn",
            measure="mean", during=[8, 10], range=[293, 340], unit="K",
            source="an air-to-air intercooler of 0.6 to 0.8 effectiveness leaves the charge some 10 to 50 K over the "
                   "outside air at full boost (makers' and tuners' intake-air-temperature logs; judgement): reported so "
                   "the over-delivery at a given boost can be judged (the lead's ask)")
        add(name="Lambda at full load under boost", scenario=SC + "p-full-load-%d.json" % fl, tel="ecu.lambda",
            measure="mean", during=[8, 10], range=[1.2, 2.0] if diesel else [0.76, 0.88], unit="ratio",
            source=("a turbo diesel at full load runs lambda 1.2 to 2 (smoke-limited; %s, ch. 10)" if diesel else
                    "a boosted petrol engine at full load is enriched to lambda 0.76 to 0.88 to cool the charge and the "
                    "turbine (factory calibrations; %s, ch. 7)") % HEYWOOD)
    else:
        add(name="Volumetric efficiency at its torque peak", scenario=SC + "p-full-load-%d.json" % fl, tel="engine.veAir",
            measure="mean", during=[8, 10], range=[0.82, 1.02], unit="fraction",
            source="a naturally aspirated four-stroke's VE at full load near its torque peak is 0.85 to 1.0 (%s, ch. "
                   "6, fig. 6-9 and the text); measured at %d rpm" % (HEYWOOD, fl))
        lo_rpm = 2000 if red_rpm > 4000 else 1200
        add(name="Volumetric efficiency low down", scenario=SC + "p-full-load-%d.json" % lo_rpm, tel="engine.veAir",
            measure="mean", during=[8, 10], range=[0.70, 0.95], unit="fraction",
            source="VE's shape: below the torque peak it falls away, the more so the hotter the cam (%s, ch. 6); "
                   "measured at %d rpm" % (HEYWOOD, lo_rpm))
    tport = [850, 1100] if diesel else [1100, 1300]
    add(name="Port gas at full load", scenario=SC + "p-full-load-%d.json" % fl, tel="exhaust.portTemperature",
        measure="mean", during=[8, 10], range=tport, unit="K",
        source=("the gas at a diesel's exhaust valve at full load, before the port and pipe cool it: some 850 to 1100 K "
                "(judgement: the header's 700 to 950 K, below, plus the port's cooling; %s, ch. 14)" if diesel else
                "the gas at the exhaust valve at full load, before the port and pipe cool it: some 1100 to 1300 K for a "
                "spark-ignition engine (%s, ch. 14; card C's target on default.json, which this follows)") % HEYWOOD)
    thead = [700, 950] if diesel else [900, 1100]
    add(name="Header gas at full load", scenario=SC + "p-full-load-%d.json" % fl, tel="exhaust.headerTemperature",
        measure="mean", during=[8, 10], range=thead, unit="K",
        source=("a thermocouple a little way down a diesel's manifold reads some 700 to 950 K at full load (makers' "
                "pre-turbine limits near 1000 K; %s, ch. 14)" if diesel else
                "a thermocouple a little way down a primary reads 900 to 1100 K at full load on a petrol engine (makers' "
                "exhaust-gas-temperature data; %s, ch. 14); n/a until the exhaust reports it") % HEYWOOD)

    # ---- the car ----------------------------------------------------------------------------------------------
    pmid = 0.5 * (plo + phi)
    qm = "p-quarter-mile-own.json" if auto else "p-quarter-mile.json"
    if not slug.startswith("tractor"):
        lb, hp = car["mass"] * 2.20462, pmid * 1.341
        et, trap = 5.825 * (lb / hp) ** (1 / 3), 234 * (hp / lb) ** (1 / 3) * 1.609344 / 3.6
        add(name="Quarter mile: time", scenario=SC + qm, tel="car.t1320", measure="last",
            range=[round(et * 0.95, 2), round(et * 1.15, 2)], unit="s",
            source="Hale's rule, ET = 5.825 (lb/hp)^(1/3) s, with the kind's published power (%.0f kW) and the build's "
                   "mass: %.2f s; 5 %% quicker to 15 %% slower for street tyres and an automatic's launch (card I's "
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
