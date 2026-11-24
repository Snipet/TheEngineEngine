# make_scenarios.py: writes package L's listening scenarios here (python3 make_scenarios.py . LEVELS PASSES); LEVELS: each camera's LEVEL
# (dB) so the pull's rms matches the tailpipe's, measured by rendering them with tee-render on content/builds/v8.json
# and comparing 4.5 to 10.5 s; PASSES: the measured passes, for the descriptions.
import json, sys, os
out = sys.argv[1]
levels = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
PASS = json.loads(sys.argv[3]) if len(sys.argv) > 3 else {}
start = [
    {"t": 0, "spec": {"env.temperature": 293.15}},
    {"t": 0, "spec": {"sound.model": 1}},
    {"t": 0, "ctl": {"mode": 1}}, {"t": 0, "ctl": {"lever": 0}}, {"t": 0, "ctl": {"clutch": 0}},
    {"t": 0, "ctl": {"throttle": 0}}, {"t": 0, "ctl": {"brake": 0}}, {"t": 0, "ctl": {"ignition": 1}},
    {"t": 0.25, "ctl": {"ignition": 2}}, {"t": 2, "ctl": {"ignition": 1}}]
asserts = [{"audio": "finite"}, {"audio": "peak", "<=": 1}]
def pull(extra):
    return start + extra + [
        {"t": 3, "press": ["shiftUp"]},
        {"t": 3.3, "ctl": {"throttle": 0.5}, "ramp": 0.2},
        {"t": 4.5, "ctl": {"throttle": 1}, "ramp": 0.1},
        {"t": 6.5, "press": ["shiftUp"]}, {"t": 8.5, "press": ["shiftUp"]},
        {"t": 10.5, "ctl": {"throttle": 0}, "ramp": 0.05}]
def write(name, desc, secs, events):
    d = {"format": "tee-scenario", "version": 1, "name": name, "description": desc, "seconds": secs,
         "events": sorted(events, key=lambda e: e["t"]), "assertions": asserts}
    json.dump(d, open(os.path.join(out, name + ".json"), "w"), indent=2)
    open(os.path.join(out, name + ".json"), "a").write("\n")
cams = ["tailpipe", "rear", "side", "engine-bay", "driver", "trackside", "chase"]
for k, c in enumerate(cams):
    lv = levels.get(c, 0.0)
    write("listen-pull-" + c, "Package L: the same pull (1st to 4th, full throttle from 4.5 s, lift at 10.5 s) heard from the %s camera, its LEVEL %+.1f dB so the pull's rms matches the tailpipe's (a comparison of character, not of level). Listen for the balance of exhaust, intake and bay, and (trackside) the pass." % (c, lv),
          13, pull([{"t": 0, "spec": {"sound.camera": k, "sound.level": round(lv, 1)}}]))
for g, name in [(2, "second"), (3, "third"), (4, "fourth")]:
    ev = start + [{"t": 0, "spec": {"sound.camera": 0, "sound.place": 1}}, {"t": 3, "press": ["shiftUp"]},
                  {"t": 3.3, "ctl": {"throttle": 1}, "ramp": 0.2}]
    t = 3.3
    for k in range(g - 1):
        t += 3.5
        ev.append({"t": round(t, 2), "press": ["shiftUp"]})
    t += 3.0
    ev.append({"t": round(t, 2), "spec": {"sound.camera": 5}})
    write("listen-pass-" + name, "Package L: a pass by trackside on a field, full throttle in %s gear: the listener is set 40 m ahead at %.1f s, the car passes it at %s; listen for the pitch falling as it passes and the ground's comb sweeping as it comes and goes." % (name, t, PASS.get(name, "?")), round(t + 7.0), ev)
write("listen-cabin-windows", "Package L: from the driver's seat, lugging in fourth from low speed (the boom through the mounts), the windows opened at 9 s: the outside comes in (the highs' share rises some 12 dB).", 14,
      start + [{"t": 0, "spec": {"sound.camera": 4, "sound.windows": 0}}, {"t": 3, "press": ["shiftUp"]},
               {"t": 3.3, "ctl": {"throttle": 0.6}, "ramp": 0.2}, {"t": 4.5, "press": ["shiftUp"]},
               {"t": 5.0, "press": ["shiftUp"]}, {"t": 5.5, "ctl": {"throttle": 1}, "ramp": 0.5},
               {"t": 9, "spec": {"sound.windows": 1}}])
places = ["hall", "field", "street", "tunnel", "garage"]
for k, p in enumerate(places):
    write("listen-place-" + p, "Package L: revving in neutral at the side camera in the %s: its early reflections and its reverberation after each lift." % p, 10,
          start[:2] + [{"t": 0, "ctl": {"mode": 0}}, {"t": 0, "ctl": {"ignition": 1}}, {"t": 0.25, "ctl": {"ignition": 2}},
                       {"t": 2, "ctl": {"ignition": 1}}, {"t": 0, "spec": {"sound.camera": 2, "sound.place": k}}]
          + [{"t": 3 + 1.5 * i, "ctl": {"throttle": 0.8}, "ramp": 0.05} for i in range(4)]
          + [{"t": 3.6 + 1.5 * i, "ctl": {"throttle": 0}, "ramp": 0.05} for i in range(4)])
