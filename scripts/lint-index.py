#!/usr/bin/env python3
"""scripts/lint-index.py: a float used as an array index without a clamp at both ends (scripts/lint.sh, rule 8).

A float's cast to int is INT_MIN on x86 for a NaN (0 on arm64, so macOS hides it), and an index below 0 or past the
end reads off the table: X-019 (sim/sound/osc.fm, a SIGSEGV in a shipped build on a Spec inside its ranges) and X-007
before it. A cast is flagged when its int is used inside [ ] within the next 8 lines and none of these holds:
  - the cast's expression is clamped at both ends: it holds both fminf( and fmaxf( (FMinus lowers them to LLVM's
    minnum and maxnum, which return the other operand for a NaN: a NaN comes out as the bound);
  - a lower check of the int follows within 4 lines (`if (i < 0) i = 0;`, as sim/core/cylinder.fm does);
  - the line says why it is safe: `// lint: index-ok: <why>`.
Better than any of them: tableIndex(x, last) (sim/core/mathx.fm). The places known on 2026-11-24 are listed in
scripts/lint-index.allow (file:line text) and reported, not failed; each leaves the list when its owner fixes it.
Prints `file:line: text` for each place flagged; exit 1 if any is not in the allow list."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent
allow_file = root / "scripts" / "lint-index.allow"
allowed = set()
if allow_file.exists():
    for l in allow_file.read_text().splitlines():
        l = l.strip()
        if l and not l.startswith("#"):
            allowed.add(l.split("  ", 1)[0].strip())

CAST = re.compile(r"(?:\bint\s+)?\b([A-Za-z_]\w*(?:\[[^\]]*\])?)\s*=\s*\(int\)\s*(.+?);")
INLINE = re.compile(r"\[[^\]]*\(int\)")
bad, known = [], []
for path in sorted((root / "sim").rglob("*.fm")):
    rel = path.relative_to(root).as_posix()
    if rel.startswith("sim/ref/"):
        continue
    lines = path.read_text().split("\n")
    for n, line in enumerate(lines):
        code = line.split("//")[0]
        if "lint: index-ok" in line:
            continue
        hits = []
        m = CAST.search(code)
        if m:
            name, expr = m.group(1), m.group(2)
            if "[" in name:
                continue  # an array of sizes or taps (cap[i] = (int)...): set, not read here
            if "fminf(" in expr and "fmaxf(" in expr:
                continue
            # a plain variable: its definition within the 3 lines before, clamped at both ends (fabsf is a lower bound)
            v = expr.strip()
            if re.fullmatch(r"[A-Za-z_]\w*", v):
                before = "\n".join(l.split("//")[0] for l in lines[max(0, n - 3) : n])
                d = re.search(rf"\b{v}\s*=\s*([^;]*);", before)
                if d and "fminf(" in d.group(1) and ("fmaxf(" in d.group(1) or "fabsf(" in d.group(1)):
                    continue
            after = "\n".join(l.split("//")[0] for l in lines[n + 1 : n + 5])
            if re.search(rf"\b{name}\s*(<|<=)\s*0\b|\b{name}\s*>=\s*0\b|\b{name}\s*=\s*{name}\s*<\s*0", after):
                continue
            used = "\n".join(l.split("//")[0] for l in lines[n + 1 : n + 9])
            if re.search(rf"\[[^\]]*\b{re.escape(name)}\b", used):
                hits.append(line.strip())
        elif INLINE.search(code):
            hits.append(line.strip())
        for h in hits:
            key = f"{rel}:{n + 1}"
            (known if key in allowed else bad).append(f"{key}: {h}")
for k in known:
    print(f"known {k}")
for b in bad:
    print(b)
sys.exit(1 if bad else 0)
