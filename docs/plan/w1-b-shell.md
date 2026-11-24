# Card B: the shell and the application

*Wave 1, started early: its first slice is half of checkpoint M1 (the other half is card A's window). Repository:
TheEngineEngine, working copy `/home/user/work/shell`, integration branch `claude/awesome-gates-ntdb58` (fetch and
merge before every push). Read [01-rules.md](01-rules.md), then [02-contracts.md](02-contracts.md) §4, §5, §7.*

**OWNS**: `shell/**`, `app/**` (the executable's `main` and its CMake), `tests/shell/**`, `scripts/run-*.sh`,
`docs/controls.md`, `docs/checkpoints/M1.md`; in the root `CMakeLists.txt` and `cmake/TeePins.cmake` only the lines
that add `shell/`, `app/`, SDL3 and the FunkGui pin (tell the keel agent's handoff file what you added; do not
reformat what is there).

## What it is

Everything between the operating system and the two things other packages make: the simulator's host
(`tee_host`, K2 and card C) and the interface's Panels (card J). It opens the audio device and runs the callback;
reads the keyboard, the mouse, a gamepad and a wheel into a `ControlsFrame`; owns the application's loop over
FunkGui's `DesktopHost` (card A) or, with no window, over `HeadlessHost`; keeps settings; loads and saves builds.

## The interfaces you define (others code against them; keep them small)

`shell/include/tee/app/Sources.h`, plain C++20, no SDL, no FunkGui, no FMinus:

```cpp
namespace tee
{
    struct ParamInfo            // one thing a person can set
    {
        std::string id, label, unit, group;
        enum class Kind { real, integer, toggle, choice } kind = Kind::real;
        double min = 0, max = 1, def = 0;
        std::vector<std::string> choices;
        bool logTaper = false; int decimals = 2; double scale = 1;
    };
    struct ParamSource          // UI thread
    {
        virtual int count() const = 0;
        virtual const ParamInfo& info(int i) const = 0;
        virtual double get(int i) const = 0;
        virtual void set(int i, double value) = 0;       // reaches the audio thread safely; never blocks
        virtual int find(std::string_view id) const = 0; // -1: none
    };
    struct TelemetrySource      // UI thread: the newest whole block's values
    {
        virtual int find(std::string_view name) const = 0;
        virtual double value(int handle) const = 0;
        virtual float outputPeak() const = 0;            // the output's level, 0 to 1
    };
    struct ControlsFrame { int ignition = 0; float throttle = 0, brake = 0, clutch = 0, handbrake = 0, steer = 0;
                           int lever = 0, shiftUp = 0, shiftDown = 0, buttons = 0, mode = 0; };
    struct ControlSink { virtual void setControls(const ControlsFrame&) = 0; };       // any thread; newest wins
    struct AudioSource { virtual void prepare(double rate, int maxBlock) = 0;         // audio stopped
                         virtual void process(float* l, float* r, int n) noexcept = 0; }; // the audio thread
    struct Player : ParamSource, TelemetrySource, ControlSink, AudioSource {};        // one engine, all four faces
}
```

`shell/` adapts K2's `tee::RefPlayer` to `Player` now (the 32 dials as `ParamInfo`s from the program's `param`
declarations and their attributes; `Shared`'s `show*` fields as telemetry; the `ControlsFrame` mapped onto the
reference's `Host` block: pedals, lever, buttons bit 4 and 5 for the paddles, the start key as its note 36) and
card C's `SimHost` when part K3 lands. Nothing above this header knows which is playing.

The Panel comes from card J through one function you declare and J defines (until J's lands, your own
placeholder: FunkGui's `Dial`s for the parameters in a grid, a start button, a throttle slider, the telemetry as
numbers; plain and correct):

```cpp
namespace tee::ui { std::unique_ptr<funkgui::Panel> makeMainPanel(tee::Player&, const AppServices&); }
```

## Slice 1: the reference, standalone (for M1; do this first)

1. **SDL3 in our tree**: the pin in `cmake/TeePins.cmake` (the same release card A pins: `release-3.4.20`), fetched
   by git, static, defined before FunkGui is added so FunkGui's desktop target uses ours.
2. **The audio device** (`shell/audio/`): an SDL3 playback stream with a callback, float stereo, the device's own
   rate, a buffer near 256 frames (ask; take what the device gives); `AudioSource::prepare` before it starts; the
   callback calls `process` and nothing else that can wait. Flush-to-zero set on the audio thread. A device that
   goes away is reopened on the default one. With `SDL_AUDIODRIVER=dummy` it runs with no sound card: that is how
   it is tested here. Measure and print the device's reported latency at start.
3. **Controls without a wheel** (`shell/input/`, `docs/controls.md`): the user asked for these by name.
   - Keyboard: Up or W the throttle and Down or S the brake, each eased toward 1 while held and back when let go
     (the times in settings); Left Shift the clutch; E or Page Up a shift up, Q or Page Down a shift down; N
     neutral; Space the handbrake; Return the start key (held: the starter; once running, a press of a second
     switches off); 1 to 6 an H-pattern gear. An automatic clutch is on by default, so the keys alone can drive.
   - Mouse: the Panel's own throttle strip (drag, or the wheel over it) and start button reach the same
     `ControlsFrame`.
   - Gamepad (SDL's gamepad interface): right trigger throttle, left trigger brake, bumpers or A and X the shifts,
     a button for the start key. Analogue, no easing.
   - Raw key-down and key-up come through `DesktopHost`'s `Options::rawEvent` (card A): `Panel::key` has no key-up.
     A key the Panel's focused text field wants is the Panel's first.
   - A wheel and pedals: leave the seam (`shell/input/Wheel.h`) and nothing behind it yet.
4. **The application** (`app/`): `TheEngineEngine`, built when `TEE_BUILD_APP` is on (it needs card A's FunkGui:
   move the FunkGui pin to the commit of its branch `claude/awesome-gates-ntdb58` that has stage 1, by commit SHA,
   and to the tag when the lead tags). It finds `sim/` beside the executable (copied there by the build; on macOS
   inside the bundle's Resources), compiles it, opens the device, opens the window, runs. A compile error is shown
   in the window, in clang's words, and the app stays up. Flags: `--info` (versions, renderer, audio device, rate,
   buffer, latency; exit), `--frames N --shot file.png` (run N frames and write the last; exit 0), `--seconds S
   --wav file.wav` (render S seconds with no device; exit 0), `--preset name`.
5. **Headless** (`tee-headless`, built always): the same application over `HeadlessHost` and no device: a script
   of inputs in, PNGs from the software rasteriser and a WAV out. This is what runs in the cloud and in `ctest`.
6. **CI artefacts**: extend the keel's workflow so each platform uploads the application with what it needs
   beside it (`sim/`, licences): `TheEngineEngine-windows-x64` (a folder with the `.exe`), `TheEngineEngine-macos-
   arm64` (an `.app`, ad-hoc signed with the entitlement `com.apple.security.cs.allow-unsigned-executable-memory`:
   FMinus's JIT maps memory without `MAP_JIT`), `TheEngineEngine-linux-x64`.
7. **`docs/checkpoints/M1.md`**: for a person on Windows and on macOS: where to download, how to run (on macOS the
   quarantine attribute a download carries, and how to clear it; or how to build from a clone with the LLVM
   FMinus's own checkout already built: `FMINUS_DEPS_DIR`), what to press, and a list of ten things to look at and
   listen for, each a yes or no.

**Slice 1 is done when** `tee-headless` renders the reference V8 through a scripted start, a rev and a stop to a
WAV and a PNG in `ctest`; the application builds on all three systems in CI and its artefacts upload; on Linux it
runs under `scripts/xvfb-run.sh` with the dummy audio driver for 300 frames and writes a picture drawn by the GPU
path. Push, and message the lead with the artefact links.

## Slice 2: the rest of the shell

8. **Settings** (`shell/settings/`): the audio device, rate and buffer; key bindings; easing times; units; the
   last build; window size. A JSON file under the user's config folder; a schema version; a bad file is renamed
   aside and defaults used.
9. **Builds on disk** (`shell/files/`): `tee::Build` (02-contracts.md §7, §8) once card C's Schema is there:
   load, save, the three parts apart, the presets folder, recent files, a dirty flag; open and save through the
   host's file dialogs.
10. **A wheel**: SDL's joystick and haptic interfaces (SDL 3.4 has a driver for Logitech wheels; the hint
    `SDL_HINT_JOYSTICK_HIDAPI_LG4FF`). Axes and buttons mapped by a table per device in settings, with a learn
    mode; the range and an autocentre spring where the device offers them. The G29's shift lights and anything SDL
    lacks: TestingPlayground's `plugin/WheelMap`, `WheelDriver`, `WheelEvdev` (JUCE-free) are the reference.
    Nothing here can be tested on a real wheel: say so, and test the mapping with a made-up device.
11. **A recorder**: the output to a WAV file while it plays, from a lock-free ring, written by a worker.
12. **Robustness**: the device unplugged; the rate changed under us; a block larger than asked; the window closed
    while the device runs; the simulator stopped by its loop budget in a development build (the output fades, the
    error shows, the app stays up).

## Not in this card

Panels and widgets (card J). The simulator's host beyond adapting it (K2, card C). Force feedback's content (wave
2). MIDI (not wanted). Packaging and signing beyond the ad-hoc artefacts (wave 2).
