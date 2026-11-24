#!/usr/bin/env python3
"""scripts/soak-summary.py <ctest log>: the soak tier's failures by kind, each with the run that failed and the one
command that reproduces it, as Markdown for the job's summary (CI's soak tier writes it; the lead's ask, so that an
owner can start from the summary without the log).

A failed combo run prints `<switches> / <build> / <scenario>: <check>: <detail>` (Catch wraps it; its lines are joined
here). Kinds, in this order: crash or bounds (a bounds check, a crash); fault or restart (the guard put the engine at
rest, a restart, FLAG_FAULT, diverged); stall (start, idle, revs, moves, stop); telemetry range (tel-range,
tel-finite); cost over 3 R (cost); other (silent, dc, click, peak, ...). Reproduce: a fuzz seed `random N` with
`tee-fuzz --seed N`, `corners N` with `tee-fuzz --seed N --corners`; a combination with
`tee-fuzz --play <build> <scenario> <switches>` (tee-fuzz is in the build's bin/)."""
import re, sys, collections

lines = open(sys.argv[1], errors="replace").read().splitlines()
start = re.compile(r"^\s{2}(.+?) / ([\w.-]+) / ([\w.-]+): ([\w:.-]+): (.*)$")
fails = []
i = 0
while i < len(lines):
    m = start.match(lines[i])
    if m:
        text = m.group(5)
        j = i + 1
        while j < len(lines) and lines[j].startswith("  ") and not start.match(lines[j]) and lines[j].strip():
            text += " " + lines[j].strip()
            j += 1
        fails.append((m.group(1).strip(), m.group(2), m.group(3), m.group(4), text))
        i = j
        continue
    i += 1

def kind(check, text):
    t = text.lower()
    if "bounds check" in t or "segfault" in t or "crash" in t:
        return "crash or bounds"
    if check == "guard" and ("restart" in t and not re.search(r"restarts 0\b", t) or "at rest" in t or "fault" in t
                             or "diverged" in t or re.search(r"(trips|stops) [1-9]", t)):
        return "fault or restart"
    if check in ("start", "idle", "revs", "moves", "stop"):
        return "stall"
    if check.startswith("tel-"):
        return "telemetry range"
    if check == "cost":
        return "cost over 3 R"
    return "other"

def repro(combo, build, scenario):
    m = re.fullmatch(r"(random|corners) (\d+)", combo)
    if m:
        return f"tee-fuzz --seed {m.group(2)}" + (" --corners" if m.group(1) == "corners" else "")
    return f"tee-fuzz --play {build} {scenario} {combo}"

order = ["crash or bounds", "fault or restart", "stall", "telemetry range", "cost over 3 R", "other"]
by = collections.OrderedDict((k, []) for k in order)
seen = set()
for combo, build, scenario, check, text in fails:
    k = kind(check, text)
    key = (k, combo, build, scenario, check)
    if key in seen:
        continue
    seen.add(key)
    by[k].append((combo, build, scenario, check, text))

total = sum(len(v) for v in by.values())
print(f"### The soak tier's failures by kind ({total})\n")
if total == 0:
    print("none")
for k, items in by.items():
    if not items:
        continue
    print(f"**{k}** ({len(items)})\n")
    print("| run | check | what | reproduce |\n|---|---|---|---|")
    for combo, build, scenario, check, text in items[:40]:
        what = text.replace("|", "/")[:160]
        print(f"| {combo} / {build} / {scenario} | {check} | {what} | `{repro(combo, build, scenario)}` |")
    if len(items) > 40:
        print(f"| ... {len(items) - 40} more | | | |")
    print()
