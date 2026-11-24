# Card A: FunkGui on the desktop without JUCE

*Wave 1, and the first gate of the whole plan (M1). Repository: **FunkGui** (`/home/user/FunkGui`), branch
`claude/awesome-gates-ntdb58`, from `b7bc1cd` (v0.27.0). One pull request to `main`; the lead merges and tags
v0.28.0. Read [01-rules.md](01-rules.md) §1 to §6 first. FunkGui's own `CLAUDE.md` was written for FCompressor's
agents on the user's Mac: by the user's word (2026-11-24) its process rules (cards in FCompressor's docs, no push,
worktree paths) do not bind this work. Its **code rules** do, and so does "never re-bless an existing golden".*

**OWNS** (in FunkGui): `CMakeLists.txt`, `CMakePresets.json`, `cmake/**`, `include/funkgui/desktop/**` (new),
`src/desktop/**` (new), `src/nojuce/**`, `tools/GalleryDesktop/**` (new), `test/**` (new `fg.desktop.*` tests and
the test CMake only), `.github/workflows/**` (new), `README.md`, `CHANGELOG.md`, `CLAUDE.md` (a short section on
the desktop target and the cloud). In `src/gpu/**` and `include/funkgui/gpu/**`: only what item 1 needs, and no
change of behaviour in a JUCE build.

## Why, and the deadline

The user can test on **Windows at about 16:00 UTC today** and on **macOS a few hours later**. Nothing in a cloud
container can open a GPU window on either, so what they test must arrive as a CI artefact they download and run.
Work in the order below and **push early**: stage 1 with artefacts for Windows and macOS is worth more at 16:00 UTC
than everything half done. Tell the lead as soon as a run has produced a Windows artefact (the run's URL).

## What exists (read these before designing anything)

- `include/funkgui/web/WebHost.h`, `src/web/WebHost.cpp`: the JUCE-free host for a browser. Your host is its
  sibling: the same order of work in a frame, the same input rules (`include/funkgui/web/WebInput.h`,
  `WebClock.h`: plain C++, reusable as they are).
- `include/funkgui/gpu/EditorHost.h`, `src/gpu/EditorHost.cpp`: the JUCE host. It has what `WebHost` lacks and you
  need: `needsDraw()`, the unchanged-frame rule (`canvas/FrameCompare.h`, `sameDrawing`), resizing, zoom, the
  teardown order, `BgfxContext::acquire/release`, the sink's `submit`, `bgfx::frame()` then
  `BgfxContext::afterFrame()` (see `src/gpu/FramePump.cpp`).
- `include/funkgui/gpu/BgfxContext.h`, `BgfxSink.h`, `NativeSurface.h`: the context takes any native window handle;
  it calls `nativeDisplay()`, `retainRenderView()` and `releaseRenderView()` from `NativeSurface.h`.
- `CMakeLists.txt` lines 84 to 90: the gate that turns bgfx off when JUCE is off. `cmake/FunkGuiTargets.cmake`: the
  `FunkGuiGpu` target (about line 455), the shader rules (478 to 544), `funkgui_compile_shaders`.
- `test/gallery/GalleryPanel.*`: the JUCE-free gallery `tools/GalleryWeb` runs. It is your first Panel.
- TestingPlayground's `.github/workflows/build.yml` (`/home/user/snipet/testingplayground`): a working recipe for
  building FunkGui, bgfx and shaderc on macOS arm64, Windows x64 (clang-cl) and Linux, with a shaderc cache, ccache
  and sccache, and GUI tests on Xvfb with Mesa's software Vulkan. Take its structure; do not copy its product steps.
- The research this follows: FMinus `docs/research/desktop/README.md` §5 and Appendix B.

## Stage 1: a window (needed for M1; do this first)

1. **`FunkGui::gpu` without JUCE.** `FUNKGUI_WITH_BGFX=ON` with `FUNKGUI_WITH_JUCE=OFF` becomes a valid
   combination. `FunkGuiGpu` then holds only the files that name no JUCE symbol (`BgfxContext`, `BgfxSink`,
   `ShaderLibrary`, `ImageGpu`, `GpuInfo`, `ShaderCheckMetal.mm` on macOS, the shaders) and the JUCE-bound ones
   (`EditorHost`, `FramePump`, `HostServicesJuce`, `A11yBridge`, `GpuPicture`, `NativeSurface.*`, cursors, display
   link) join only when JUCE is on. **With JUCE on, every target has the same sources, flags and definitions as
   v0.27.0**: prove it (compare the generated source lists of the `agent` preset before and after, or state why you
   could not configure it here and let CI's JUCE job prove it).
2. **SDL3** as a pinned dependency of a new option `FUNKGUI_WITH_DESKTOP` (OFF by default; needs bgfx; a normal
   variable, as the other options): fetched by git at release `release-3.4.20` with its commit asserted as the other
   pins are, static, only video, events, joystick, gamepad, haptic, audio and dialog subsystems needed; skipped if
   the consumer already defined `SDL3::SDL3`. On Linux force SDL's X11 video driver (FunkGui's Vulkan path is X11
   only; Wayland through XWayland), and list the system packages SDL3 needs in the README.
3. **`FunkGui::desktop`**: `include/funkgui/desktop/DesktopHost.h` and `src/desktop/`. The public header includes no
   SDL header (forward-declare `struct SDL_Window; union SDL_Event;`). Shape, to refine as the code asks:

   ```cpp
   namespace funkgui::desktop
   {
       struct Options
       {
           std::string title;
           int themeIndex = 0;
           bool resizable = true;            // only if the Panel says resizable()
           // Called on the main thread with every SDL event before the host reads it; true: the host ignores it.
           // How an application takes key-up, gamepad and joystick events, which Panel's structs do not carry.
           std::function<bool(const SDL_Event&)> rawEvent;
           // Called once a frame on the main thread before Panel::tick (WebHost's beforeTick).
           std::function<void()> beforeTick;
       };

       class DesktopHost final : public HostServices
       {
       public:
           DesktopHost(Panel&, Options);
           bool open(std::string& why);      // SDL, the window, bgfx; false and why on any failure
           int  run();                       // the loop, until the window closes; the process's exit code
           bool pump();                      // one turn of the loop, for an application that owns its own
           void requestFrame();              // any thread: wake the loop and draw
           void close();
           SDL_Window* window() const;
           // The last frame submitted, read back from the GPU as straight RGBA8 (tests, screenshots)
           bool capture(std::vector<std::uint8_t>& rgba, int& w, int& h, std::string& why);
       };
   }
   ```
4. **The frame**, as `EditorHost` does it and `WebHost` documents it: advance the cadence, follow the preferences'
   revision, `beforeTick`, `Panel::tick(dt)`, fill `FrameInfo` (dpi from the window's pixel density, seconds as the
   sum of dts, the theme's ground and gamma), record, skip a frame equal to the last one drawn unless `needsDraw()`,
   submit through `BgfxSink`, `bgfx::frame()`, `afterFrame()`. Time only from the loop's clock into `tick(dt)`.
5. **The loop**: while the Panel wants full rate, frames paced by the swap chain's vsync; otherwise 12 Hz (the
   other hosts' idle rate), sleeping in `SDL_WaitEventTimeout` between; `Panel::idle()` at 10 Hz or more. Draw
   during a live resize (SDL's event watch for exposed and resized events) so the edge and the picture stay
   together as far as SDL and bgfx allow; note what you see on Xvfb and leave macOS's behaviour for the user's test.
6. **The window handle for bgfx**: Windows the `HWND` from SDL's window properties; Linux the X11 display and window
   number; macOS the `CAMetalLayer` from `SDL_Metal_CreateView` and `SDL_Metal_GetLayer` (plain C calls, so
   `src/desktop/` needs no Objective-C), or the `NSWindow` from the properties if bgfx at the pin wants that.
   Supply the three `NativeSurface.h` functions the context calls. If a build with JUCE and the desktop both on
   would define them twice, make the two exclusive in CMake for this release and say so in the README.
7. **Input** to the Panel's structs: pointer move, down, drag, up, exit, double click (SDL's `clicks`), `popup`
   (right button; control-click on macOS), `button`, modifiers (`cmd` is the GUI key on macOS, as
   `commandKeyIsMeta` says), wheel in the units `WebInput.h`'s `wheelFromDom` produces, pinch as `magnify`, keys
   and text (`SDL_EVENT_TEXT_INPUT` gives `Key::character`), all in logical px.
8. **`tools/GalleryDesktop`**: `funkgui_gallery_desktop`, the gallery in the window. Flags for machines and for
   people: `--frames N` (run N frames, then exit 0), `--shot file.png` (write the last frame, from `capture`),
   `--section name`, `--info` (print `funkgui::version()`, `gpuInfo()`, the renderer, the window's size and scale,
   one line each, and exit). It prints one line `fg.desktop: ok renderer=<name> frames=<n>` on a clean exit.
9. **CI**, `.github/workflows/desktop.yml`: on push to `main` and `claude/**`, on pull requests, and by hand. Jobs
   for `macos-15` (arm64), `windows-latest` (clang-cl) and Ubuntu: configure the new `desktop` preset (JUCE off,
   bgfx on, desktop on), build, run the tests; on Linux run `funkgui_gallery_desktop --frames 120 --shot` under
   Xvfb with Mesa's software Vulkan and keep the PNG. **Upload the gallery executable of each platform as an
   artefact** (`funkgui-gallery-desktop-<platform>`, with whatever it needs beside it). **Caches**: shaderc by
   platform and bgfx commit (the build's slowest part), compiler objects by ccache or sccache, the fetched
   sources; a warm run should build in a few minutes. Pin every action to a commit SHA.
10. A `desktop` preset and `desktop-verify` workflow preset in `CMakePresets.json`.

**Stage 1 is done when**: the `nojuce` and `desktop` presets build with no warnings here and `tools/verify.sh`
passes for each; the Linux CI job has a PNG of the gallery drawn by the real GPU path; the Windows and macOS jobs
build and upload their artefacts. Push, open the pull request as a draft, and message the lead with the artefact
links and exact run instructions for a person on Windows and on macOS (three lines each).

## Stage 2: the host's services

11. **`HostServices`**: clipboard text; `chooseFiles` through SDL's dialogs (asynchronous; the callback on the main
    thread); the cursor polled from `Panel::cursor()` onto SDL's system cursors; `setUnboundedDrag` through
    relative mouse mode; zoom (`zoomPercent`, `setZoomPercent`, steps, `zoomFits`: the window resizes, as
    `EditorHost` does); `nowSeconds`; `nudgeFullRate`; `themeIndex`; `announce` and `a11yFocus` as no-ops, said so.
12. **Menus drawn by the Canvas** (`showMenu`, `dismissMenus`): an overlay in the same window over the Panel's
    frame, in the theme's look: rows, separators, headers, checked and radio items, shortcuts' text, disabled rows,
    submenus, keyboard (arrows, Return, Escape, type-ahead), a click outside dismisses; it stays inside the window.
    It is drawn with the Canvas so `HeadlessHost` and the software rasteriser can test it: factor it so a test can
    drive it without SDL. `services()` then reports menus, file chooser and clipboard.
13. **A PNG writer without JUCE** in `src/nojuce/WritePng.cpp` (stored deflate blocks need only CRC-32 and
    Adler-32; no new dependency), so `HeadlessHost::writePng` works in every JUCE-free build.
14. **A file backend for `UiPreferences`** the host installs: a small text file under SDL's preferences path for
    `FUNKGUI_PREFS_FOLDER`.
15. **Tests**, `fg.desktop.*`, self-registering as the others: input conversion (pure functions, no window); the
    menu overlay's layout, keys and fingerprints through `HeadlessHost`; the PNG writer against a known file; the
    preferences backend in a temporary folder; one GPU test that runs only when a display is set.
16. **Docs**: README (a Desktop section: the target, the options, the packages on Linux, what was run where),
    CHANGELOG v0.28.0, the version bump, `CLAUDE.md`'s code rules extended to `desktop/` (an SDL header appears only
    in `src/desktop/` and `tools/GalleryDesktop/`).

**Stage 2 is done when** every item has its test green in CI on all three systems, the pull request is ready, and
the handoff lists exactly what was proven on which system and what waits for the user's eyes: the look of a live
resize on macOS and Windows, Retina scale, a second monitor, the file dialogs, the trackpad's pinch.

## Not in this card

A second window, popup windows, the system menu bar, accessibility, input methods, drag and drop, a bgfx bump,
Wayland, text beyond the atlas. Leave a line in the README for each. Audio and game devices are TheEngineEngine's
shell (card B), which takes SDL's events through `Options::rawEvent`.

## Budget and courtesy

4 cores, shared with other agents: `-j3`. shaderc is about 2,000 CPU-seconds: build it once into
`/home/user/.tee-deps/tools/` and point `FUNKGUI_SHADERC` at it in every later configure; CI caches its own.
