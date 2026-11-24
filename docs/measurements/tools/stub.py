#!/usr/bin/env python3
# docs/measurements/tools/stub.py: for measurement only. In a COPY of sim/, empties the bodies of every method of the
# named modules (their fields kept, so every caller still compiles): an approximation of a model "defined out" (never
# compiled), for its code size, compile time and the hot loop's cost without it. The output is not the program's.
#   stub.py <copy of sim> Module[,Module...]
import re, sys, glob
root, names = sys.argv[1], set(sys.argv[2].split(','))
for path in glob.glob(root + '/**/*.fm', recursive=True):
    src = open(path).read()
    out, i, changed = [], 0, False
    for m in re.finditer(r'^module (\w+)\n\{\n', src, re.M):
        pass
    lines = src.split('\n')
    res, k, inmod = [], 0, None
    while k < len(lines):
        ln = lines[k]
        mm = re.match(r'^module (\w+)$', ln)
        if mm:
            inmod = mm.group(1)
        if ln.startswith('};'):
            inmod = None
        sig = re.match(r'^    (?!return)([\w\*]+)\s+(\w+)\(.*\)(\s*\[\[[^\]]*\]\])?\s*$', ln)
        if inmod in names and sig and k + 1 < len(lines) and lines[k + 1] == '    {':
            ret = sig.group(1)
            j = k + 2
            while lines[j] != '    }':
                j += 1
            body = {'void': '', 'float': ' return 0.0f;', 'int': ' return 0;', 'bool': ' return false;',
                    'double': ' return 0.0;'}.get(ret, ' return 0;')
            res.append(ln)
            res.append('    {' + body + ' }')
            k = j + 1
            changed = True
            continue
        res.append(ln)
        k += 1
    if changed:
        open(path, 'w').write('\n'.join(res))
