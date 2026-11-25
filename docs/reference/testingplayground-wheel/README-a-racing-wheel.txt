- **A racing wheel.** The synth's Standalone plays a Logitech G29 (the wheel, its pedals and the Driving Force Shifter,
  one USB cable) as a MIDI input: plugged in, it shows among the MIDI inputs in **Settings** as "Logitech G29 Racing
  Wheel", switched like one (its choice kept with the others), with a short line under it when there is something to
  say. Set the switch on the wheel to **PS3** (at PS4 it is not read, and the card says so): a G29 started there without
  Logitech's software is in its compatibility mode, which the app switches to its full mode as it is switched on
  (Linux's hid-lg4ff does the same; the wheel leaves and comes back, "switching to its full mode" meanwhile). It tries
  once each time it is switched on: a wheel that does not come back in its full mode (or comes back in its
  compatibility mode, or is plugged in again in it) is not switched again until you turn it off and on, and the card
  says so. Then the app turns it to 900 degrees with a moderate centring spring, as a game does, and stops the spring
  when it is switched off or the app quits; a crash or a force-quit sends nothing, and the spring stays on until the
  wheel is unplugged (the card says so too when the wheel refuses the stop). While it is on, it sends MIDI on channel
  1, each control only when it changes (a report's values together, at most 125 times a second), and the program
  reads it in `host.fm`:
  - CC 16 / 48: the steering, 14 bits (8192 the centre): `host->steering`, -1 full left to 1 full right
  - CC 17 / 49, 18 / 50, 19 / 51: the throttle, the brake and the clutch, 14 bits (0 released): `host->throttle`,
    `host->brake`, `host->clutch`, 0 to 1
  - CC 20: the shifter's gear & 0x7F (127 reverse, 0 neutral, 1 to 7): `host->gear`, -1, 0, 1 to 7
  - CC 21 to 24: the buttons, a bit each, 7 bits a control (bits 0-6, 7-13, 14-20, 21-27): `host->buttons`; a G29's:
    0 cross, 1 square, 2 circle, 3 triangle, 4 right paddle, 5 left paddle, 6 R2, 7 L2, 8 share, 9 options, 10 R3,
    11 L3, 12 +, 13 -, 14 dial right, 15 dial left, 16 return, 17 PS, 18 to 21 the d-pad's up, right, down, left (the
    shifter's positions are the gear, not bits: 25 buttons and the d-pad's four would take more than 28)
  - the start key: the PS button also plays note 36 (C2) at velocity 100 while it is held, so a program whose voice
    runs only while it has a note (an engine that cranks on its first key and keeps itself going) starts without a
    keyboard

  The program lights the wheel's five shift lights (the pairs over its hub, lit from the outside in) with a field of
  its own named `shiftLights`, the main module's or else Shared's (Shared's when the sound runs on between notes, as an
  engine does; a voice's is the newest sounding voice's, as a display reads it, and none while no voice sounds): a
  float from 0 to 1 lights that share of them (floor(5 x): 0.2 one, 0.6 three, 1 all five), an int is their mask (bit 0
  the outermost pair, the first a tach lights; bit 4 the middle). A tach's: `shiftLights = (rpm - 0.75f * redline) /
  (0.25f * redline);` all five at the redline, and at the limiter 1 and 0 by turns about eight times a second flash
  them. The app looks 40 times a second while the wheel is read and sends the lights only when they change; a program
  without the field never touches them, and one that drops it turns them off. Switched off or the app quitting, they
  go off (with the spring); unplugged, they go with the wheel; a crash leaves them lit until the wheel is unplugged. A
  field of that name that is no float, int or bool (an array, a dial) is a note on the run.

  A 14-bit control's MSB resets its LSB to 0, as MIDI says: a source that sends only the MSBs (a knob on CC 17) reads 7
  bits. A pedal's first 1 % of travel reads released (its pot's jitter at rest sends nothing), and a change of an axis
  smaller than 3 of its 16,384 steps waits until it grows (unless it comes back to rest). Until a report from the wheel
  says otherwise, each control is at rest (what macOS holds before the first report is not the wheel's). Switched off
  or unplugged, the wheel sends what puts these back at rest (centred, released, neutral, no button, the start key's
  note off), so nothing stays held; after the audio device restarts (which drops the MIDI waiting for it) and after
  **Reset to Default State** it sends its whole state again. The values stay until a control changes them, and a reset
  of the Engine puts them at rest. The same control changes work in the AU and the VST3 from any MIDI source that
  sends them (the effect gets no MIDI: its fields stay at rest). Claude moves them in a render's scenario (its wheels
  `steering`, `throttle`, `brake`, `clutch`, `gear` and `buttons`, the mask, sent as the wheel sends it, the start
  key's note 36 too while bit 17 is down), and hears
  them in listen and in a recording's report. Which HID element is which is `Source/plugin/WheelMap.h`'s table,
  checked on no wheel yet: **wheel-dump**
  (`Tools/wheel-dump/`, never installed) shows what a connected G29 has. `cmake --build build-dev --target
  wheel-dump`, then `build-dev/wheel-dump` lists its elements (usage page and usage, logical minimum and maximum, the
  value macOS holds and its timestamp, 0 until a report came, the table's row for each) and prints, ten times a
  second, the elements that changed, what the synth will read and the MIDI the app would send (Ctrl-C ends it);
  `--list` only lists, `--switch` switches a G29 in compatibility mode to its full mode (as the app does), `--spring`
  sets the range and the spring while it runs (off again on Ctrl-C, SIGTERM and SIGHUP, and while Ctrl-Z has it
  stopped), `--leds` tries the shift lights: each alone for a second (bit 0 to bit 4), all five, none, then the
  throttle lights them as `shiftLights` would, flashing all five at the floor, each report printed as it goes (off
  again as the spring is). A row of the table is the whole fix when one is wrong. The lights' report (`f8 12`, the
  mask, `00 00 00 01`: hid-lg4ff's command, its last byte the one in doubt) was written from memory and is checked on
  no wheel yet: `WheelMap.h`'s `kLightsLast` is the place to change it.

  On Linux the kernel's own driver, hid-lg4ff, drives the wheel: it switches a G29 out of its compatibility mode by
  itself (where it was told not to, the app writes "native" to its `alternate_modes` file), and the app
  (`plugin/WheelEvdev.h`) reads the wheel's input device (`/dev/input/event…`, never grabbed: a game may read it too),
  sets the range through hid-lg4ff's `range` file (only when it is not 900 already: the kernel sets 900 as it takes the
  wheel), the spring as force feedback's autocentre on the input device, and the shift lights through hid-lg4ff's LED
  files (`…::RPM1` to `::RPM5`, bit 0 the first). A wheel plugged in or out while the app runs is seen at once. The
  input device is yours while you sit at your desktop (systemd-logind gives a joystick to the user at the seat); the
  `range`, `alternate_modes` and LED files are root's, so without a udev rule the lights stay dark (the card says so),
  as does a range another program left at other than 900. A rule that gives them to you, in
  `/etc/udev/rules.d/99-g29.rules` (then `sudo udevadm control --reload` and plug the wheel in again; written for this
  README, not tried on a wheel):

  ```
  ACTION=="bind", SUBSYSTEM=="hid", DRIVER=="logitech", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c24f|c294", \
    RUN+="/bin/sh -c 'chmod 0666 /sys%p/range /sys%p/alternate_modes /sys%p/leds/*/brightness'"
  ```

  A wheel whose input device the app may not open at all is listed, the card saying so. `evtest` (its own package)
  shows what the wheel sends there; wheel-dump is macOS's.
- The theme (the top bar's moon or sun: Graphite or Paper), the zoom (the status bar's pill) and the code's text size
  (Cmd-=, Cmd--, Cmd-0) are machine-wide preferences, each product's own.

## The chat

The rounded card at the right is a conversation with Claude, through the [Claude API](https://docs.claude.com) with
your own Anthropic API key (billed to you), or, if you choose, with another provider's model (**Other model
providers**, below: OpenAI, OpenRouter, Google Gemini, or a model server on this Mac such as Ollama or LM Studio). Claude sees the plugin's files, whether they play, the last compile's
messages and the dials (sent with a message when they changed; a file it has already seen goes as one line, not again
in full), and it can act on them: read, search (a text or a pattern, in one file or in all of them at once, the
lines found numbered, with up to ten lines around each, or only how many each file has, and on past the first 40),
write, edit (a passage, or every match of a text or a pattern, in one file or in all of them: a name changed
everywhere, never a netlist's drawing lines, at most 2,000 at once, all or none), rename and delete the files (the
editor's tabs follow, and Cmd-Z in a file undoes its change; an edit of several files at once, all or none, is one
step that Cmd-Z in any of their tabs undoes in all of them), add the library's building blocks, run the program (RUN; the compile's
messages come back to it), set the dials, measure what the program puts out, see the panel (view_panel: a picture of
it, drawn headless on the GPU as your screen draws it, shaders included, which goes to Anthropic with the request, so
that Claude can check its layout and fix it; the session keeps no picture; it can also draw the panel in a state that
is not playing now, which changes nothing you hear: dials and fields at values Claude names, `{"shared->gear": 12,
"out": -6}`, so it can check a dashboard in twelfth gear or a warning light lit, or the state the program reaches at a
moment of a render made offline with view_sound's settings (not its trial edits), the displays, the shaders' inputs and the dials as they are
then, the text saying what each display shown reads), draw a circuit (edit_circuit, below), see the sound (view_sound), hold its work to checks it sets and tune the dials toward goals
(below, **Checking its work**), read the CPU load and the output guard's alerts, and ask you questions you answer with
a click. Ask the effect for an effect ("a tape delay with wow and flutter") and the synth for a sound ("a warm detuned
pad", "a plucked string"), either for a change ("make the filter resonate more", "a slower attack"), an explanation
("what does this program do?"), or help with an error or a click. It knows FMinus, the plugin's contract, the library
(each file's header) and how to build a large program from it, and it lays the dials out in a panel (its instructions
are in `Source/plugin/ChatPrompt.cpp`).

**Files in and out.** Claude brings files into the program when you ask: files you attach in Claude Desktop, or a
file or a folder you name on your Mac (`~/Music/my-synth`). It reads only the program's kinds there (`.fm`, `.cir`,
`.gui`, `.sc`, `.sh`), each as text (a file with a NUL byte, or bytes that are not UTF-8, is refused) of 256 KB at most and
with a name the editor takes; a folder's other files are left out and named, and so are the read-only copies a folder
sync wrote there (FOLDER, below). It is all or none, one change Cmd-Z undoes: a file of a name the editor has replaces
it, the others come as new tabs (at most 16), the program stays the first, CR LF become LF and a character outside
ASCII (which the editor does not hold) becomes `?`, and Claude is told of each (the characters counted). Your files
are only read. The other way, Claude writes the editor's files as they are to a new folder in `~/Music/Funk/<the plugin's
name>/Programs`, of the name it gives (with a number after it when that is taken: an export adds a folder and never
replaces a file; two files whose names differ only in case, one file on a Mac's disk, are refused), with the program's
samples beside them as they were added, and says where. Both are write_file's (`files`, `from`, `to`;
`Source/plugin/ChatImport.cpp`), which also writes several files as one change, and adds a sample by its path (`path`;
an audio file in a folder brought in is left out and named, for `path` to add).

Claude edits a netlist's schematic as you do: **edit_circuit** sends commands a line each (add a part, with or without
a place; place, connect, disconnect, set a value or a key, rename a part or a node, delete, lay out again), which the
plugin applies together through the same operations the Schematic view's gestures use (`fminus/CirDoc.h`), all or
none, as one step you can undo in the tab, the parts it changed flashing in the drawing; it writes only while the
netlist is still the text it read, so a gesture of yours in that moment is never overwritten (the commands run once
more on your text, and a second clash fails, telling Claude to try again; a gesture made just after Claude's write,
before the tab shows it, is undone and the strip asks you to do it again). A part or a terminal with no place keeps
where your view drew it (the view's session pins, kept in memory). Its answer says what each command did, the
netlist's lines that changed, what FMinus's circuit compiler says of the new text, and the netlist itself when it is
short, with its version as a write's (an edit_file that expects the version from before it is told the change was
Claude's own; one after your undo of Claude's edit is told the change was yours). A longer netlist counts as seen when
its answer lists every line that changed and Claude had seen the text it edited, so the next message does not send it
again. view_panel's view **schematic** is a picture of the drawing as your view draws it (at 100 %, every name and
value shown, the last run's marks), with each part's place and pins in words; it touches nothing of your view. A
netlist Claude rewrites whole without its `*@` lines keeps your drawing (Claude is told), and Claude is taught to leave
those lines alone, or to edit a part's line to move it.

Claude keeps your **projects** too, when you ask ("save this", "keep it as a preset called Warm", "go back to version
3", "try the Dark preset"), or at a milestone you agreed to ("save a version each time it works"): never on its own,
and never silently. Its **projects** tool (Claude Desktop has the same one) lists the projects (the open one and
whether it has unsaved changes, then each with its category, newest version and presets; or one project's versions
with their notes and its presets), saves the editor's files and the dials as a new version of the project open (or of
a project it names: a new one when none has the name), with a note of what changed ("Claude: a brighter filter
(program.fm +12 -3, 2 dials)", which PROJECTS lists) and a category if you gave one (with nothing new to save, the
category alone), saves the dials as a preset (with a category), applies a preset of the project open, and opens a
project or one of its versions. It is safe by design: a save only adds (a project keeps its newest 100 versions, as
your own saves do; a factory project is never written: a save makes your own copy, its presets with it); a preset of
the same name is replaced only when you asked for that, and Claude is told its old values; opening a project while the
editor holds changes nothing saved holds (files or samples changed since they were opened or saved, a dial moved from
the version's or the preset's value, or from its default with no project open), or applying a preset over dials moved
so (your files stay as they are), is refused, naming them, unless you said to drop them;
while you compare in A/B nothing is saved and no preset applied (Keep a side or Close first); and nothing is ever
deleted (that stays yours, in PROJECTS). What Claude opens or applies is yours: its checks never send it on to change
it, and opening another project clears them (they were for the program before). Each save, open and preset shows: the
status line says it once, in its sentence case as every notice there ("Claude saved version 5 of engine"), the chat's
card has its row, PROJECTS and the explorer read the lists again (an open PROJECTS card too: the version and preset
you chose stay chosen, and a Delete or another confirmation it was asking for is withdrawn, so you choose again),
Claude Desktop learns of the plugin's own chat's saves from its change notices (get_status's `since`), and a project
Claude opens is one step you can undo in the editor. A project the plugin's own chat opens takes the conversation along,
as a save does (from no project it moves there; from another project a copy goes, that one keeping its own), so the
answer stays in front of you; one Claude Desktop opens is as if you had opened it, its own chats showing.

