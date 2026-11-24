# Card A: FunkGui on the desktop without JUCE (handoff, 2026-11-24 13:20 UTC)

*Filed by the lead from the agent's report: FunkGui's agent could not write in this repository. Repository
Snipet/FunkGui, branch `claude/awesome-gates-ntdb58` at c0bc6ed (21 commits on b7bc1cd), merged to `main` at 19d06ab
by pull request 38 (13:19). Version 0.28.0. The tag `v0.28.0` is the owner's to push.*

**CI** (run 45400413371 at c0bc6ed, all green):
- windows-x64 (clang-cl, warnings as errors, static CRT): no warning; 44 of 44 tests (6 `fg.desktop.*` and 38
  others); the gallery ran 120 frames on Direct3D 11 (the runner's software adapter), its PNG read back from the GPU.
- macos-arm64 (macos-15): no warning; 70 of 70; the gallery on Metal (the VM's Apple GPU), PNG read back.
- linux-x64 (Xvfb, openbox, lavapipe): 70 of 70; the gallery on Vulkan, PNG read back.

**Done, by test**: `fg.desktop.input` (75 rows: SDL to Panel input rules), `fg.desktop.files` (file-dialog patterns),
`fg.desktop.png` (the PNG writer against a known file), `fg.desktop.menu` (the overlay's layout, pointer, keys,
type-ahead, pixels, both themes), `fg.desktop.prefs` (text-file preferences), `fg.desktop.host` (75 rows on a real
hidden window: open, frame, capture against SoftRaster, input wiring, clipboard, unbounded drag, a menu in the GPU
frame, zoom, a resizable Panel, close and reopen). Also: `FunkGui::gpu` without JUCE; SDL3 pinned at release-3.4.20
by git; the `desktop` and `desktop-verify` presets; `funkgui_gallery_desktop` (`--frames`, `--shot`, `--section`,
`--info`, `--list`, `--resize`); `.github/workflows/desktop.yml` (actions pinned by SHA; caches for sources, shaderc
and the compiler).

**Proven where**: the cloud container (Linux, lavapipe on Xvfb): `desktop` 70/70, `nojuce` 66/66, `web` 66/66, no
warnings. JUCE builds unchanged, proved by comparing compile commands (`agent`: 2,296; `agent-gui`: 3,198; identical
but for the version string; 46 new commands in 3 new test targets); no JUCE preset was built whole. The three
renderers draw the same gallery frame (Metal against Direct3D 11 within 2 a channel; lavapipe within 6). Real
hardware: only the owner's Windows 11 machine (primitives, sharp at 150 %, near 0 % CPU at rest, the services
section, `--resize` by dragging and maximised).

**Not done, waits for a person**: anything on a real Mac (live resize, Retina, trackpad scroll and pinch, file
dialogs); a second monitor of another scale; cursors; vsync pacing. On macOS SDL does not say trackpad or notched
wheel: both are treated as a trackpad, so a notched mouse wheel will scroll slowly there.

**For the lead**: `fg.desktop.menu`'s 40 new golden rows (`frame.theme<0|1>.dpi<1|2>.*`) are candidates, reported
MISSING; no existing golden was touched.

**Measurements**: the gallery at rest 3.3 % of a core on lavapipe (which draws every frame); CI warm: Linux 2 min,
macOS 2 min, Windows 9 min; cold shaderc 11 to 23 min; the Windows executable 5.1 MB, system DLLs only.

**One change a consumer sees**: a native JUCE-free build that wants no GPU must set `FUNKGUI_WITH_BGFX=OFF` (the
`nojuce` and `web` presets do). JUCE and `FUNKGUI_WITH_DESKTOP` exclude each other.

**Wanted outside its paths** (each worked around): `tools/check-headers.sh` needs a rule for JUCE-bound `gpu/`
headers with bgfx and no JUCE; `tools/GalleryApp/CMakeLists.txt` assumes bgfx means JUCE; `tools/GalleryProbe.cpp`
and the tests `canvas_image`, `canvas_parity`, `canvas_shader`, `harness_self`, `prefs_backend`, `web_prefs` need
POSIX, so they, `fg.gallery.*` and `fg.headers` are not built on Windows.

**Findings**: FunkGui's own suite had never been built on Windows; Git for Windows' CRLF checkout breaks the golden
parser (CI sets `core.autocrlf` false). No FMinus issue met.
