# Checkpoints: what only the user can do

No cloud session can open a window on a GPU, play a sound, or hold a wheel. Three checkpoints put the work in
front of a person. Each has a page with exact commands, written by the agent whose work it tests, under
`docs/checkpoints/`.

## M1: a window and a sound (2026-11-24: Windows about 11:00 local, macOS in the afternoon)

**What is tested**: FunkGui's new host (card A) and the application's first slice (card B).

**In two steps, so that a failure says which half failed:**

1. **FunkGui's gallery in an SDL3 window**, nothing of TheEngineEngine in it: an artefact of FunkGui's CI run
   (`funkgui-gallery-desktop-windows-x64`, `-macos-arm64`). Run it. Yes or no, each:
   - the window opens and shows the gallery, sharp on a high-density screen;
   - it resizes, and the picture follows the edge without tearing or stretching (say how it looks while dragging);
   - the pointer, the wheel or trackpad, and the keyboard reach the widgets;
   - it idles without spinning a core, and animates smoothly when something moves;
   - it closes cleanly; `--info` prints the renderer (Direct3D 11 on Windows, Metal on macOS).
2. **TheEngineEngine** (`TheEngineEngine-windows-x64`, `-macos-arm64`): the reference engine in our own window.
   - it starts (the first launch compiles the engine: say how long the wait is);
   - Return cranks and starts it; Up or W is the throttle; it sounds as the plugin does;
   - no crackle or dropout at the default buffer; say which device and rate `--info` reports;
   - the dials move the sound as they did in the plugin;
   - closing it stops the sound at once.

**If the application is not ready** by the time you test, step 1 still stands alone, and there will be a WAV file
rendered by the same code for your ears.

**What to send back**: for each line, yes or no and a sentence; a screenshot of anything odd; on macOS, whether
the download had to be un-quarantined (the page says how) or you built from a clone.

## M2: the end of wave 1

The application with its control pages and wave 1's physics. A **listening pack**: scenarios rendered on your Mac
by `tee-render` (nothing binary is committed), each archetype with each new subsystem off and on: does the turbo
spool and flutter like one; does the cam change the idle the right way; does the two-stage change sound right;
do the overrun bangs follow their dial; does the car launch, spin and hook up believably. A contact sheet of every
screen. Your wheel, if card B's seam has anything behind it by then. Your corrections decide wave 2's order.

## M3: the end of wave 2

Every screen with its view, the shaders, the cameras and the places, two-strokes, diesels and bikes, a signed
build on your Mac.
