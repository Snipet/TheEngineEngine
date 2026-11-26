# Controls

*What drives the engine in TheEngineEngine, as built (shell/input/). A Logitech G29 is read on macOS and Linux (below).*

Everything below ends in one `ControlsFrame` a frame (shell/include/tee/app/Sources.h), which is all the engine
sees: the keys, a gamepad and the Panel's own controls are merged, and for each pedal the largest wins.

## Keyboard

Keys are read by their place on the board, not by the letter printed on them, so W, A, S, D are where a hand
expects them on any layout.

| Key | What it does |
|---|---|
| Up, or W | The throttle. It eases down while the key is held (0.30 s for the whole travel) and back up when it is let go (0.18 s). |
| Down, or S | The brake, the same way (0.20 s down, 0.12 s up). |
| Left Shift | The clutch pedal (0.10 s down, 0.30 s up). Only needed with the automatic clutch off. |
| Space | The handbrake. The reference engine's car has none: the key does nothing yet. |
| Return, or keypad Enter | The start key (below). |
| E, or Page Up | A shift up. |
| Q, or Page Down | A shift down. |
| N | Neutral. |
| 1 to 6 | That gear. |
| Z | A truck's splitter: a press turns it on (the high half of each gear) or off. |
| X | A truck's range: a press turns the high range on or off. |
| J | The engine brake (a compression brake): a press turns it on or off. Its stage (Off, Low, Medium, High) is a setting, the ECU's brake switch; with that Off the button gives High. |

**The truck's switches** (Z, X, J) latch like the switches they stand for (a splitter's thumb switch, a range's
flip lever, a dash switch): they reach the simulator as the held bits of its `ctl.buttons` (BTN_SPLITTER,
BTN_RANGE, BTN_ENGINE_BRAKE) until pressed again. The eighteen-speed's own lever does not drive the vehicle's
gearbox yet.

**The start key.** Press it and the starter cranks the engine; it goes on cranking by itself until the engine
runs (two seconds at the most: an engine that will not catch is left on, not cranking; press again). Hold it for a second while the engine runs and the engine is switched off. A tap is enough to start: a press
shorter than a frame is still held for 0.15 s.

**The gears.** The automatic clutch is on (there is no switch for it in the window yet). With it, E and Q are the
paddles of a paddle-shift car: the gearbox works its own clutch, pulls away by itself when the throttle goes down in
first, and never stalls. A gear key (1 to 6, N) presses the shift up or down for you, once every 0.3 s, until the
gearbox is in that gear; it gives up after twelve presses (a downshift the gearbox refuses because it would take the
engine past its redline). With the automatic clutch off, a gear key moves an H-pattern lever and the clutch is yours
(Left Shift): the Driver has this path and its test, and nothing in the window turns it on yet.

The times are `tee::input::DriverSettings`; they move to the settings file with the bindings in the shell's
second slice.

## Files and the audio device

Taken before the keys above (so Ctrl+S saves and does not brake). On a Mac, Cmd in place of Ctrl.

| Key | What it does |
|---|---|
| Ctrl+N | A new build, every field at its default. With unsaved changes it asks first (discard, or save them first). |
| Ctrl+O | Open a build (a `.json` of `docs/formats.md`), through the system's file chooser. |
| Ctrl+S | Save the build to its file; an untitled one, or a shipped one (the first build, an archetype), asks for a file of your own. |
| Ctrl+Shift+S | Save it as another file. |
| F2 | The File menu: the above, the recent files, the builds of the library (shipped and your own), and a part (engine, car, tune) loaded from or saved to a file apart from the rest. |
| F9 | The Audio menu: the device by name (or the system's default), the rate (or the device's own), the buffer asked for; the QUALITY (Full: the engine at 96 kHz inside, the default; Economy: 48 kHz inside, about half the load, the top octave lost; a change compiles the engine again and crossfades to it, the engine cranked back if the key was on; a build on the edge of lighting, such as a diesel whose charge only just lights, may come back on the other branch, lit or misfiring, as after any reload); and the recorder, which writes what is heard to a WAV file in the user's folder (`recordings/` beside `builds/`). |

The top bar shows the build's name with a star while it has unsaved changes. A build still unsaved when the window
closes is kept as the build "Unsaved changes" in your folder of builds, and opens at the next start; otherwise the
build last opened or saved opens. A first launch (nothing remembered) opens the first build, `content/builds/first.json`:
a whole engine with every model on, its name in the top bar, clean, and a TRY FIRST card with its notes' opening
paragraph (closed once, it is not shown again: `tryFirstSeen` in the settings). The other archetypes are in GARAGE.
`--reference` opens no build: the simulator's defaults, every model switch at Reference, for comparisons; `--preset
NAME` opens that build instead of either. The device, the rate, the buffer and the recent files are remembered in the
settings file (`shell/settings/Settings.h`). A device that is unplugged: the system's default plays in its place,
and the named one is taken back when it returns.

## Mouse

In the frame, the cockpit's **START**, its mode (bench, dyno, drive) and its three pedal bars (throttle, brake,
clutch: press and drag) reach the same frame as the keys.

The reference's panel's own controls reach the same frame: its **START** button is the start key (hold it a second to switch
off), its **throttle strip** is the throttle while it is pressed and dragged, and its gear control asks for a gear
as the keys do. In the plain Panel (`--panel plain`) the strip is the tall bar at the left: press and drag it, and it
springs back when let go; the wheel over it sets a throttle that stays where it is put, until it is wheeled back to
nothing or the strip is pressed.

A knob turns by a drag up or down, or by the wheel over it. In the plain Panel, Shift makes the drag fine and a
double click puts a dial back at its default.

## Gamepad

Through SDL's gamepad interface, so any pad SDL knows. Analogue, with no easing.

| Control | What it does |
|---|---|
| Right trigger | The throttle |
| Left trigger | The brake |
| Left stick, left and right | Steering (nothing reads it yet) |
| Right bumper, or A (the bottom face button) | A shift up |
| Left bumper, or X (the left face button) | A shift down |
| Start | The start key |

The first pad plugged in is the one read; when it is unplugged, the next. Nothing here has been tried with a pad
in hand: the code was written against SDL's documentation and compiled.

## A wheel and pedals

**A Logitech G29** (the wheel, its pedals and the Driving Force Shifter on one USB cable) is read on **macOS** (IOKit)
and **Linux** (the kernel's own driver, hid-lg4ff) with no software of Logitech's installed, and on **Windows** read
only, as a plain joystick (below). The code is TestingPlayground's, where it drove the owner's G29
(`shell/input/g29/SEED.tsv`).

**What is verified where.** macOS: the owner reports (26 Nov, on `main` at b557c4a1) that their G29 "worked well with
the Mac app": the owner's report, not a test of ours, and it says nothing in particular of the lights or the shifter.
Linux and Windows: never yet run with a real wheel. No test here opens a device.

- **The switch on the wheel at PS3.** At PS4 it is not read (the status line says to set it to PS3). At PS3, without
  Logitech's software, a G29 starts in a compatibility mode: the application switches it to its full mode once each
  time the wheel is turned on in the application ("wheel: switching to its full mode"; the wheel leaves and comes back).
- **Read, it is set up as a game sets it up:** 900 degrees lock to lock and a moderate centring spring. Turned off, or
  the application quitting, the spring stops and its shift lights go out. A crash or a force-quit leaves the spring on
  until the wheel is unplugged.
- **macOS asks** whether the application may read input devices (Privacy & Security, Input Monitoring) the first time;
  refused, the status line says where to allow it.

| On the wheel | What it does |
|---|---|
| Throttle, brake, clutch pedals | The pedals: each is the larger of the wheel's and the keys' or a gamepad's (its first 1 % of travel reads released). |
| Right paddle / left paddle | Shift up / shift down (a press each), by default. |
| The shifter | The gate it is in: with the automatic clutch off, the H-pattern lever itself (the clutch pedal is yours); with it on, the gear is sought through the gearbox's own shifts, as a gear key does (a sequential or an automatic box too). Out of every gate: neutral. |
| PS button | By default, the start key, held as long as it is held (a press starts a stopped engine; held on a running one, off). |
| The steering | Read but not driven: the car runs in a straight line. |
| Its five shift lights | The DRIVE view's own count (the same five steps below the redline), all five flashing on the limiter. |
| The other buttons | Editing the engine (below): the inspector's hands, by default. |

**Editing from the wheel.** The buttons the wheel does not drive with edit the engine while it runs: no mode, the
pedals, paddles, shifter and PS drive exactly as above. Each is a binding (moved as below), by default:

| On the wheel | What it does (once a press) |
|---|---|
| D-pad up / down | The inspector's row up / down; held, it repeats after 0.35 s, then about 12 a second |
| D-pad left / right | The inspector's page before / after (ENGINE 1, ENGINE 2, VOICES, ...) |
| L2 / R2 | The workspace before / after (GAR, ENG, VLV, AIR, EXH, TUNE, CAR, DYNO, DRV, LSN, SCP) |
| The red dial, a detent left / right | The focused value down / up a nudge (as the mouse wheel over it: a hundredth of its travel; a choice or a whole number, one step) |
| Minus / plus | Ten nudges down / up |
| SHARE, held | Makes a value's nudge fine (a thousandth, as Shift) |
| The dial's return, cross | Activate: a switch flips, a group folds or unfolds, a choice steps on, a table opens |
| Circle | Back: a table closes, else the focus is let go |
| Square / triangle | Undo / redo |
| L3 | The focused field to its default |
| OPTIONS | The "changed only" filter on and off |
| R3 | Free |

The buttons are read once a frame: a dial's detent is one nudge, and detents faster than the frames (two between two
frames) count once; at most one command a button a frame. Editing from the wheel works where its buttons are read:
macOS and Linux today; on Windows once the owner's probe has given its table (below). Until the frame takes the
commands (its `edit()`, being built), the console says once "editing from the wheel waits for the frame".

**On and off:** the settings page's WHEEL row (on macOS, Linux and Windows; its status line under it), or F9, "Read the racing wheel
(Logitech G29)": on by default, kept in the settings file (`wheelOn`). Off,
the device is left open (shared: another program may read it too) but nothing is read from it or sent to it. The
status line says what the wheel is doing ("wheel: G29 read", "wheel: set the switch on the wheel to PS3", ...), and the
console prints each change. On, the wheel's buttons also edit the engine (Editing from the wheel, above).

**On Linux** the kernel's hid-lg4ff drives the wheel: it switches a G29 at PS3 to its full mode by itself, and the
application reads its input device (`/dev/input/event…`, never grabbed), sets the range through hid-lg4ff's `range`
file (only when it is not 900 already: the kernel sets 900 as it takes the wheel), the spring as force feedback's
autocentre, and the shift lights through hid-lg4ff's LED files (`…::RPM1` to `::RPM5`). The input device is yours
while you sit at your desktop; the `range`, `alternate_modes` and LED files are root's, so without a udev rule the
lights stay dark (the status line says so). A rule that gives them to you, in `/etc/udev/rules.d/99-g29.rules` (then
`sudo udevadm control --reload` and plug the wheel in again; TestingPlayground's, not tried on a wheel here):

    ACTION=="bind", SUBSYSTEM=="hid", DRIVER=="logitech", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c24f|c294", \
      RUN+="/bin/sh -c 'chmod 0666 /sys%p/range /sys%p/alternate_modes /sys%p/leds/*/brightness'"

`evtest` (its own package) shows what the wheel sends there; `tee-wheel-dump` is macOS's.

**The buttons can be moved.** F9, "Wheel buttons": choose a driving action (start, shift up, shift down, neutral,
handbrake, splitter, range, engine brake) or an edit (the table above), then press the wheel's button for it within
10 s ("wheel: press the button for ..."; a button already held counts once it is let go and pressed again; SHARE is
not taken while it is the modifier). The button loses whatever it did before; a driving action is also taken from the
button that had it (it lives on one), an edit may stay on others too (activate is on two). The status line says the
new binding, and while it listens the buttons drive and edit nothing. "The defaults" puts back the map above. The
bindings are kept in the settings file (`wheelButtons`, as `2:4=shiftup,...,18=e.rowup,...`: a button's bit, the table
of `shell/input/g29/G29.h`, and an action's or an edit's name); a pair it cannot read is left out with a line on the
console. A `wheelButtons` saved before editing from the wheel (no `2:`) keeps every binding in it and gains each edit
on a button it left free: driving wins, an edit never takes a button the owner bound. A binding on SHARE makes it the
owner's, no longer the modifier. The pedals, the steering and the shifter are fixed by the wheel's table, not bound.

**On Windows** the wheel is only read, never written to: Windows sends a device's output reports at the length its
descriptor gives (17 bytes), and the G29 takes its commands (the switch to its full mode, the range, the spring, the
lights) as 7-byte reports, so without Logitech's software nothing can reach it (SDL 3.4 turns its own Logitech driver
off there for the same reason). The application reads it as a plain joystick through SDL and sends it nothing:

- Without Logitech G HUB the wheel stays in its compatibility mode (its switch at PS3): the steering, the throttle
  and brake, the buttons and the paddles are expected; the clutch and the shifter are not expected (a Driving Force
  EX has neither). The status line: "G29 read as a joystick: pedals and buttons; the clutch, the shifter, the range,
  the spring and the lights need Logitech G HUB".
- With Logitech G HUB, G HUB puts it in its full mode and sets the range, the spring and the lights itself; every
  control is expected to read ("G29 read (Logitech G HUB): range, spring and lights are G HUB's"). The application
  never needs G HUB: it says what G HUB would add.
- **Not mapped yet.** Which axis and button is which on Windows has not been seen on a real wheel, so no mode has a
  table yet (`shell/input/g29/G29Joystick.h`): the wheel is seen and its mode said ("G29 seen (compatibility mode):
  its controls are not yet mapped on this system: run TheEngineEngine --probe-input and paste what it prints"), and
  nothing drives from it. `TheEngineEngine --probe-input`, run once in each mode, gives the table.

**Not yet:** force effects beyond the spring (and on Windows, any). A G29 is never opened as a gamepad (SDL's Logitech driver is turned off, so it
cannot reset the spring).

**To see what the wheel sends** (macOS): `tee-wheel-dump` beside the application (`build-release/bin/tee-wheel-dump`)
lists the G29's HID elements with the row of the application's table each one maps to, then prints the elements that
change and what the application reads of them (the pedals, the steering, the gear, the buttons). `--list` only lists;
`--switch` switches a G29 at PS3 to its full mode; `--spring` sets 900 degrees and the spring while it runs; `--leds`
tries each shift light, then lets the throttle light them. Ctrl-C ends it, with the spring and the lights off.

**To see what the computer sees** of any controller: `TheEngineEngine --probe-input` (macOS:
`./TheEngineEngine.app/Contents/MacOS/TheEngineEngine --probe-input`). It opens no window; it lists every game
controller SDL sees (name, ids, SDL's type, axes, buttons, hats, force feedback) and prints their changes for 60 s
(`--probe-seconds S`). It prints to the Terminal only and sends nothing anywhere.

## What the engine does with them

The application plays the simulator (`sim/engine.fm`) in the vehicle (the frame's mode is MODE_DRIVE), in neutral:
the pedal is the throttle, and the key is a car's (off, on, start). `--ref-panel` (or `--engine ref`) plays the
reference engine (`sim/ref`) instead, in its own panel, started in **DRIVE CAR**: in a car, in neutral, so the pedal
is the throttle and the engine idles until it is switched off. The reference's `drive` dial (DRIVE, on its panel's
SOUND & DRIVE page) changes that:

- **CAR** and **TRUCK**: as above. The start key cranks only in neutral, or with the clutch pedal down.
- **KEYS** and **PITCH** (the reference as a synthesiser plays it): the engine runs from the start key until the
  start key is held for a second, and then for as long again as the IDLE FOR dial says (the reference switches an
  engine off that nobody is holding a key for). The throttle is the reference's "throttle by hand". There is no
  gearbox, so the shifts and the gear keys do nothing.

## For a machine

`--workspace ID[:PAGE]` opens the frame on a workspace (garage, engine, valves, air, exhaust, tune, car, dyno,
drive, scope), `--size WxH` gives the window or the picture a logical size, `--scale S` a headless picture's pixels
a logical px, and `--dyno-wait S` has tee-headless wait for the dyno ribbon's first curve before it draws:

    tee-headless --workspace exhaust --size 1280x800 --scale 2 --frames 120 --script tests/shell/scripts/workspace.txt --shot out/exhaust.png

A scripted window (`--frames`, `--shot`) opens no build unless one is named (`--first-build FILE`, `--preset NAME`):
the simulator's defaults, as `--reference`, so its pictures and timings do not change with a content file.

`tee-headless` (and the window with `--script`) reads the same keys from a script, by name: `w`, `s`, `up`, `down`,
`e`, `q`, `n`, `space`, `return`, `enter`, `lshift`, `pageup`, `pagedown`, `1` to `6`. `app/Script.h` has the
format (`expect sent PATH` reads a Spec field as the host was sent it, `quality full|economy` changes the
QUALITY; the window checks a script's `expect`s as tee-headless does); `tests/shell/scripts/` has scripts that start,
rev, drive and stop the engine.
