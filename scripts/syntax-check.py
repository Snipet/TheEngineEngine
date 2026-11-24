#!/usr/bin/env python3
"""scripts/syntax-check.py <build tree> [file.cpp ...]: the C++ twin of lint.sh's FMinus check, a compile with
-fsyntax-only (no code made: about two seconds), with each file's own flags from <build tree>/compile_commands.json.

Always: one unit that includes every public header of the host (host/include/tee/**/*.h), with host/src/Program.cpp's
flags. Then each .cpp named (lint.sh names those that differ from the integration branch: what is about to be
pushed, after the merge), with its own flags; a file the build tree does not compile is skipped. Three at a time.
Exit 1, with the compiler's first errors, when anything does not compile."""
import json, os, pathlib, shlex, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor

root = pathlib.Path(__file__).resolve().parent.parent
tree = pathlib.Path(sys.argv[1])
cc = json.loads((tree / "compile_commands.json").read_text())
by_file = {str(pathlib.Path(e["file"]).resolve()): e for e in cc}

def syntax(entry, source):
    args = shlex.split(entry["command"]) if "command" in entry else list(entry["arguments"])
    k = max(i for i, a in enumerate(args) if a.endswith(("clang++", "g++", "c++", "clang-cl", "cl.exe", "clang", "gcc", "cc")))
    out, skip = [], False
    for a in args[k:]:
        if skip:
            skip = False
            continue
        if a in ("-o", "-c", "-MF", "-MT", "-MQ", "/Fo"):
            skip = True
            continue
        if a in ("-MD", "-MMD") or a.startswith(("-o", "/Fo")) and a not in ("-o",):
            continue
        if pathlib.Path(a).name == pathlib.Path(entry["file"]).name:
            continue
        out.append(a)
    out += ["-fsyntax-only", source]
    r = subprocess.run(out, cwd=entry["directory"], capture_output=True, text=True)
    return r.returncode, r.stderr

jobs = []
program = by_file.get(str((root / "host/src/Program.cpp").resolve()))
if program:
    headers = sorted((root / "host/include/tee").rglob("*.h"))
    unit = tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False)
    unit.write("".join(f'#include "{h}"\n' for h in headers))
    unit.close()
    jobs.append((f"the host's {len(headers)} public headers", program, unit.name))
for f in sys.argv[2:]:
    e = by_file.get(str((root / f).resolve()))
    if e:
        jobs.append((f, e, e["file"]))

with ThreadPoolExecutor(3) as pool:
    results = list(pool.map(lambda j: (j[0],) + syntax(j[1], j[2]), jobs))
bad = [(name, err) for name, rc, err in results if rc != 0]
for name, err in bad:
    first = [l for l in err.splitlines() if "error" in l][:3]
    print(f"syntax: {name}: does not compile:\n  " + "\n  ".join(first or err.splitlines()[:3]))
print(f"syntax: {len(jobs) - len(bad)} of {len(jobs)} compile", file=sys.stderr)
sys.exit(1 if bad else 0)
