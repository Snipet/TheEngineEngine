#!/usr/bin/env python3
"""scripts/apply-help.py: paste the help sentences of docs/manual/help.tsv into sim/spec/*.fm as [[ui::help("...")]].

    scripts/apply-help.py                 # every spec file; prints what it added, file by file
    scripts/apply-help.py --file car.fm   # only sim/spec/car.fm
    scripts/apply-help.py --check         # changes nothing: says what it would add, exit 1 if anything

The table (docs/manual/help.tsv, written by the content session) is `path<TAB>sentence`, a path as the schema writes it
(spec.car.gears). Each path is resolved through the sources: from module Spec (sim/spec/spec.fm) field by field, each
field's type naming the module the next part is in. The sentence goes on the field's declaration, right after its
[[ui::label(...)]], on a line of its own. A field that already has a ui::help is never touched (the owner's text
wins); a path that resolves to nothing is reported, not guessed. Quotes and backslashes in a sentence are escaped.
Re-run it when the table grows: what is already there is left alone.
"""
import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SPEC = ROOT / "sim" / "spec"
TABLE = ROOT / "docs" / "manual" / "help.tsv"

MODULE = re.compile(r"^module\s+([A-Za-z_]\w*)\s*$|^module\s+([A-Za-z_]\w*)\s*\{", re.M)
# a field's declaration at the start of a statement: a type, a name, then an attribute, an extent or the end
DECL = re.compile(r"^(\s*)([A-Za-z_][\w:]*)\s+([A-Za-z_]\w*)\s*(?=\[|;)", re.M)
LABEL = re.compile(r'\[\[ui::label\("(?:[^"\\]|\\.)*"\)\]\]')


def modules():
    """module name -> (file, start, end) of its body, for every module in sim/spec/*.fm"""
    found = {}
    for path in sorted(SPEC.glob("*.fm")):
        text = path.read_text()
        for m in MODULE.finditer(text):
            name = m.group(1) or m.group(2)
            open_at = text.find("{", m.start())
            depth, i = 0, open_at
            while i < len(text):
                if text[i] == "{":
                    depth += 1
                elif text[i] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            found[name] = (path, open_at + 1, i)
    return found


def fields_of(text, start, end):
    """name -> (type, statement start, statement end) of the fields declared at the module's own level"""
    body = text[start:end]
    out = {}
    depth = 0
    i = 0
    stmt = 0
    while i < len(body):
        c = body[i]
        if c == '"':                      # skip a string
            i += 1
            while i < len(body) and body[i] != '"':
                i += 2 if body[i] == "\\" else 1
        elif c == "/" and body[i:i + 2] == "//":
            nl = body.find("\n", i)
            i = len(body) if nl < 0 else nl
            continue
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                stmt = i + 1              # a function's body ended
        elif c == ";" and depth == 0:
            piece = body[stmt:i + 1]
            # the declaration, after any comment lines before it
            lines = [l for l in piece.split("\n")]
            k = 0
            offset = 0
            while k < len(lines) and (lines[k].strip() == "" or lines[k].strip().startswith("//")):
                offset += len(lines[k]) + 1
                k += 1
            m = DECL.match(piece, offset)
            if m and "(" not in piece[offset:m.end()]:
                out[m.group(3)] = (m.group(2), start + stmt + offset, start + i + 1)
            stmt = i + 1
        i += 1
    return out


def resolve(path, mods, texts):
    """spec.a.b.c -> (file, statement start, statement end) of c's declaration, or None"""
    parts = path.split(".")
    if parts[0] != "spec":
        return None
    module = "Spec"
    for k, name in enumerate(parts[1:]):
        if module not in mods:
            return None
        file, start, end = mods[module]
        fields = fields_of(texts[file], start, end)
        if name not in fields:
            return None
        type_, s, e = fields[name]
        if k == len(parts) - 2:
            return file, s, e
        module = type_
    return None


def escape(sentence):
    return sentence.replace("\\", "\\\\").replace('"', '\\"')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", help="only this file of sim/spec/ (car.fm)")
    ap.add_argument("--check", action="store_true", help="change nothing; exit 1 if anything would be added")
    args = ap.parse_args()

    rows = []
    for line in TABLE.read_text().splitlines()[1:]:
        if "\t" in line:
            p, s = line.split("\t", 1)
            if s.strip():
                rows.append((p.strip(), s.strip()))

    mods = modules()
    texts = {f: f.read_text() for f in SPEC.glob("*.fm")}
    edits = {}            # file -> [(statement start, statement end, path, sentence)]
    missing, kept = [], 0
    for p, s in rows:
        where = resolve(p, mods, texts)
        if where is None:
            missing.append(p)
            continue
        file, start, end = where
        if args.file and file.name != args.file:
            continue
        if "ui::help(" in texts[file][start:end]:
            kept += 1
            continue
        edits.setdefault(file, []).append((start, end, p, s))

    added = 0
    for file, items in sorted(edits.items()):
        text = texts[file]
        for start, end, p, s in sorted(items, reverse=True):   # from the end, so the offsets hold
            stmt = text[start:end]
            label = LABEL.search(stmt)
            if label is None:
                missing.append(p + " (no ui::label to put it after)")
                continue
            indent = re.match(r"\s*", stmt).group(0).replace("\n", "")
            at = start + label.end()
            rest = text[at:]
            # the help on a line of its own after the label; what followed the label follows the help
            help_line = indent + "    " + '[[ui::help("' + escape(s) + '")]]'
            if rest.startswith("\n"):
                after = rest
            else:
                tail, nl, more = rest.lstrip(" ").partition("\n")
                if len(help_line) + 1 + len(tail) <= 120:
                    after = " " + tail + nl + more
                else:   # too long for one line: what followed the label goes on a line of its own
                    after = "\n" + indent + "    " + tail + nl + more
            text = text[:at].rstrip(" ") + "\n" + help_line + after
            added += 1
            print(f"{file.relative_to(ROOT)}: {p}")
        if not args.check:
            file.write_text(text)

    print(f"apply-help: {added} added, {kept} already had a help text, {len(missing)} not found", file=sys.stderr)
    for p in missing:
        print(f"apply-help: not found: {p}", file=sys.stderr)
    if args.check and added:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
