#!/usr/bin/env python3
"""Copies the panel code from a TestingPlayground checkout into ui/kit/ and writes ui/kit/SEED.tsv.

Run once, by hand, when the seed is taken (or to see what a newer TestingPlayground would bring: run it into a
scratch folder and diff). It does only what is mechanical: the first-line comment that names the origin, the
namespace (playground and playground::ui both become tee::ui) and the include paths. Every other difference from
the source was made by hand afterwards and is listed in docs/ui-kit.md; running this again over ui/kit/ would undo
those edits, so it refuses to overwrite a file unless --force is given.

    import-seed.py <testingplayground checkout> <commit> <ui/kit directory> [--force]
"""
import hashlib, os, re, sys

FILES = [
    # (source path under Source/, our path under ui/kit/)
    ("fminus/GuiSpec.h", "gui/GuiSpec.h"), ("fminus/GuiSpec.cpp", "gui/GuiSpec.cpp"),
    ("fminus/PanelSpec.h", "gui/PanelSpec.h"), ("fminus/PanelSpec.cpp", "gui/PanelSpec.cpp"),
    ("editor/GuiLayout.h", "gui/GuiLayout.h"), ("editor/GuiLayout.cpp", "gui/GuiLayout.cpp"),
    ("editor/GuiItems.h", "gui/GuiItems.h"), ("editor/GuiItems.cpp", "gui/GuiItems.cpp"),
    ("editor/Looks.h", "gui/Looks.h"), ("editor/Looks.cpp", "gui/Looks.cpp"),
    ("editor/Items.h", "gui/Items.h"), ("editor/Items.cpp", "gui/Items.cpp"),
    ("editor/DialLayout.h", "gui/DialLayout.h"), ("editor/DialLayout.cpp", "gui/DialLayout.cpp"),
    ("editor/DisplayItem.h", "gui/DisplayItem.h"), ("editor/DisplayItem.cpp", "gui/DisplayItem.cpp"),
    ("editor/Dropdown.h", "gui/Dropdown.h"),
    ("editor/Scroll.h", "gui/Scroll.h"), ("editor/Scroll.cpp", "gui/Scroll.cpp"),
    ("editor/TabStrip.h", "gui/TabStrip.h"), ("editor/TabStrip.cpp", "gui/TabStrip.cpp"),
    ("editor/GuiView.h", "gui/GuiView.h"), ("editor/GuiView.cpp", "gui/GuiView.cpp"),
    # What the files above lean on, cut down by hand to the part they use (docs/ui-kit.md says what was cut):
    ("editor/ChatStyle.h", "gui/Style.h"),
    ("dsp/Controls.h", "gui/Controls.h"),
    ("dsp/Sources.h", "gui/Sources.h"),
    ("dsp/Meters.h", "gui/Meters.h"),
    ("editor/PanelView.h", "gui/GuiPanel.h"), ("editor/PanelView.cpp", "gui/GuiPanel.cpp"),
    # Shaders: the text side (what a fragment source is made of), the library, the font.
    ("fminus/Shaders.h", "shaders/ShaderText.h"), ("fminus/Shaders.cpp", "shaders/ShaderText.cpp"),
    ("fminus/ShaderLib.h", "shaders/ShaderLib.h"), ("fminus/ShaderLib.cpp", "shaders/ShaderLib.cpp"),
    ("editor/ShaderFont.h", "shaders/ShaderFont.h"), ("editor/ShaderFont.cpp", "shaders/ShaderFont.cpp"),
    ("editor/nogpu/ShaderLibraryNone.cpp", "shaders/ShaderLibraryNone.cpp"),
]

INCLUDES = {src: ours for src, ours in FILES if src.endswith(".h")}

def convert(text, src, ours, commit):
    def include(m):
        name = m.group(1)
        return '#include "kit/%s"' % INCLUDES[name] if name in INCLUDES else m.group(0)
    text = re.sub(r'#include "([^"]+)"', include, text)
    text = text.replace("playground::ui::", "tee::ui::").replace("namespace playground::ui", "namespace tee::ui")
    text = text.replace("namespace playground::sandbox", "namespace tee::ui")
    text = text.replace("namespace playground", "namespace tee::ui").replace("playground::", "tee::ui::")
    head = "// ui/kit/%s: from TestingPlayground, Source/%s at %s (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.\n" % (ours, src, commit)
    return head + text

def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    root, commit, out = sys.argv[1], sys.argv[2], sys.argv[3]
    force = "--force" in sys.argv
    rows = []
    for src, ours in FILES:
        raw = open(os.path.join(root, "Source", src), "rb").read()
        rows.append("ui/kit/%s\tSource/%s\t%s\t%s\n" % (ours, src, commit, hashlib.sha256(raw).hexdigest()))
        dest = os.path.join(out, ours)
        if os.path.exists(dest) and not force:
            continue
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        open(dest, "w", newline="\n").write(convert(raw.decode("utf-8"), src, ours, commit))
    open(os.path.join(out, "SEED.tsv"), "w", newline="\n").write(
        "# our path\tTestingPlayground's path\tcommit\tsha256 of the file as it was there\n" + "".join(sorted(rows)))

main()
