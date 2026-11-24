# The `.gui` language

*What a panel file says and how it is read here. The reader is `ui/kit/gui/GuiSpec.cpp` (from TestingPlayground at
`47bac40`: `ui/kit/SEED.tsv`); its layout is `GuiLayout.cpp`, its drawing `GuiItems.cpp` and `Looks.cpp`. The
first section is the language as the reader's own header describes it; the second is what is different in this
application. `tests/ui/panels/kinds.gui` uses every element in every look and is the file to copy from.*

## The language

A panel is one text file whose name ends in `.gui`. The reference engine's is `sim/ref/panel.gui`.

```
The language, a line an element (or several, between ;), a block of children between { and }:

  element argument argument key=value key="value" {
      child ...
  }

An argument or a value is a name (cutoff, log), a "text", a number (12, -0.5) or a colour (#e07b39). // and /* */
are comments. The file's elements stack top to bottom, as in a column; `panel` may hold them instead, with the
panel's own colours (background=#1d1b19 accent=orange).

Containers (props: gap= between children, pad= inside, align= across: stretch (the default), start, center, end;
justify= along: start, center, end, between, around; grow= its share of the room left in its parent; width= and
height= at least that many px; wrap: a row's children onto more lines when it is too narrow):
  row { }                 its children left to right
  column { }              top to bottom
  grid cols=4 { }         in rows of cols, each column as wide as its widest child
  section "FILTER" { }    a frame, its title cut into the top edge (color=, layout=row (the default) or column)
  box { }                 a filled frame without a title (color=, layout=column (the default) or row)
  tabs { page "OSC" { } page "FILTER" { } }   pages, one shown at a time, chosen by the tabs over them
  spacer                  room (width=, height=; without them it grows)
  divider                 a line across a row or down a column (color=, thickness=)
  label "TEXT"            text (size=small, normal, large, title or px 10 to 44; align=left, center, right; color=)
Any element: margin= (room around it), self= (its own align=), minwidth= maxwidth= minheight= maxheight=.

Looks. section and box: look=frame (a section's default), card, inset or plate; radius=, fill=, border= (a colour or
none), borderwidth=, shadow=; a section's titlecolor=, titlealign=, titlesize=. row and column: fill=, border=,
radius=. tabs: look=tabs (the default), bar (a header bar), pills or side (a list at the left). panel: background=,
gradient= (the ground's colour at the bottom), accent=.

Controls, each a parameter's (one control a parameter), with label="..." (else the parameter's name) and color=:
  knob cutoff             size=small, medium, large or a face's px (24 to 240); look= (style=) modern (the default),
                          vintage, flat, minimal, ring, led, chicken, soft or dot; arc=, track=, cap=, pointer=line,
                          dot, triangle or none, pointercolor=, thickness=, sweep= (degrees), ticks=, tickcolor=,
                          bipolar, labelpos=top, bottom or none, value=always, hover or never, valuecolor=,
                          labelcolor=, shadow=; for a float also unit="Hz" decimals=1 scale=100 (shows 0..1 as
                          0..100) taper=log (a frequency's or a time's: every octave the same turn; its range must be
                          above 0), which the host's text and its automation follow too
  slider level            vertical, or `slider pan horizontal`; length= px; unit=, decimals=, scale=, taper=; arc=,
                          track=, cap=, thickness=, bipolar, labelpos=, value=
  switch sync "OFF" "ON"  a bool, or an int of two values; the texts name them
  buttons wave "SAW" "SQUARE" "TRI"   a lit button a value (2 to 8), in a column, or a row: buttons wave horizontal
  menu mode "A" "B" "C"   a drop-down of an int's values (or a bool's), the texts their names
  xy cutoff resonance     a pad: x the first parameter, y the second; size= px (120)
  envelope attack decay sustain release   the ADSR shape its four parameters make, drawn (width=, height=)
  keyboard from=48 keys=25   an instrument's keys, which play notes (octaves= instead of keys=; color=)
  shader "glow.sc" inputs="cutoff env out"   a fragment shader of the program's own files, drawn by the GPU over
                          its cell (width=, height= its size, else 200 x 120; it stretches as a scope does), four
                          at most: its inputs (up to
                          16: parameters, fields, in, out) are its in0..in15 (fminus/Shaders.h); it may #include
                          the library (funk/text.sh, funk/shapes.sh: fminus/ShaderLib.h) and the program's .sh
                          files; the compiler builds it, and a shader that does not compile is a note on its own line
  area x=padX y=padY     room that draws nothing (width=, height=; else 120 x 80): what the pointer does there,
                          its pointer properties below, alone

The pointer, on area, shader, row, column, grid, section, box, label and the displays (the elements that take no
press of their own; a container gets the presses its children do not take, so a knob in it still turns), each the
name of a parameter it writes:
  x=  y=                  a float: the pointer's place across (left 0, right 1) and up (bottom 0, top 1), as a knob's
                          turn maps its range and taper; drag=absolute (the default: they jump to the press and
                          follow it) or relative (they move by its travel, 200 px the range, Shift ten times finer);
                          release=keep (the default) or spring (back to their defaults when let go)
  down=                   a bool, int or float: its highest value while pressed, its lowest when let go
  wheel=                  a float or an int, nudged by the wheel as a knob is
  hover=                  a panel-only parameter (one after the program's 32nd: no host parameter), its highest while
                          the pointer is over it
  cursor=                 the pointer's shape there: hand, cross, grab, leftright or updown
One press is one gesture of x, y and down together (the host's automation, the journal, undo), a double-click puts x
and y back to their defaults, a right-click opens the first one's dial menu (MIDI learn). A parameter a pointer writes
may have a control too; one without comes in no section of the rest. 16 elements with pointer properties at most.
A shader's inputs may also be the pointer over it, with no parameter: pointer.x and pointer.y (0 to 1 across and up
its rect, the last place while the pointer is elsewhere), pointer.down (1 while it is pressed), pointer.hover (1
while the pointer is over it): in_pointer_x ... in its code, counted among its 16. A dial drawn by a shader:
  shader "dial.sc" inputs="cutoff pointer.hover pointer.down" y=cutoff drag=relative cursor=updown width=72 height=72

A knob's or a slider's live=field marks where a field of the program has the parameter now (a hidden display's tap;
Shared's too, shared->x, as a display names it).

Classes and defaults, anywhere in the file (read first): `class "big" size=72 arc=yellow` gives its properties to an
element with class="big" (several: class="big warm", in order); `defaults knob look=ring` gives them to every knob.
An element's own properties come last, so they win; a class's or the defaults' property an element does not take is
left out without a note.

Displays, of `in` or `out` (the plugin's audio: an effect's input, the output) or a float, double, int or bool field
the program writes: the main module's (an instrument's from its newest sounding voice), or the module Shared's through
the main module's Shared* field, `shared->level` (its one Shared, whether a voice sounds or not), or one element of an
array of them, an index a dimension (`bands[2]`, `grid[1][3]`: PanelSpec.h's displaySource), with label= (else
the source's name; Shared's field's own, LEVEL), color=, and min= max= for a field (0 and 1 when unsaid):
  meter out   vu out   led gate   number cutoffHz   scope out   spectrum out
scope and spectrum (an analyser of in or out) take width= and height=. A number shows an int's or a bool's field as
a whole number, and takes a knob's unit=, decimals= and scale= for a field (`number rpm unit="rpm" decimals=0`); its
cell is wide enough for a sign, four digits, its decimals and unit, and a longer readout is drawn smaller.

A parameter that no control names comes last, in a section without a title. What the plugin cannot use is a note,
"panel.gui:12:5: note: ...", and the rest stands.
```

## What is different here

**What a control names.** A control names a *parameter* by its id: `tee::ParamInfo::id` of the `tee::ParamSource`
the panel is bound to (`shell/include/tee/app/Sources.h`). Today those are the reference's 32 dial names (`size`,
`cam`, `turbo`, ...); after part K3 they become Spec paths such as `boost.level` (card J2 extends the reader's
names for that: a name is letters, digits and `_` today, so a dotted path is not yet read). A parameter's range,
default, choices, unit, decimals, scale and taper come from its `ParamInfo`; what the file says (`unit=`,
`decimals=`, `scale=`, `taper=log`, the texts after a `menu`, `switch` or `buttons`) is used in its place. A
label the file does not give is the parameter's `label`.

**How many.** One panel shows at most 64 parameters (`kMaxControls`), 32 displays (`kMaxDisplays`), 4 shaders,
16 elements with pointer properties, 400 elements. A source with more than 64 parameters shows its first 64 and
the panel's notes say so. (J2's generated pages lift this by making a page a panel of its own.)

**What a display names.** A display, a `live=` mark and a shader's input name a *telemetry value*: a name the
`tee::TelemetrySource` answers `findValue` for. `shared->showRpm`, as the reference's panel writes it, asks for
`showRpm`: what stands before `->` is dropped. Every value is a number (a float to the reader); an array's
element (`bands[2]`) is not read here. A name the source does not have is a note and reads 0. A name that is both
a parameter's id and a telemetry value's is the parameter.

**`in` and `out`.** There is no input: `in` is left out with a note, as it was for an instrument. `out` is the
output's peak (`TelemetrySource::outputPeak`); a VU meter is fed the RMS a sine of that peak would have (the
source gives no RMS). A `scope` or a `spectrum` of `out` needs the output's samples, which `TelemetrySource` does
not carry: they draw a flat line until the application passes `GuiPanel::Options::scope`.

**`keyboard`.** Read and laid out, so the panel keeps its shape, and never drawn, hit or listed: there is no
MIDI. `GuiPanel::keyboardRects()` says where it would be; the reference's panel puts its strip there (a start
button, a throttle bar, the gear: `ui/ref/Strip.h`).

**`hover=`.** In the plugin it needed a parameter no host could automate. Here no parameter is a host's, so any
parameter may be named.

**Shaders.** A `shader "x.sc"` is built when the application is built, not while it runs: the file must be
listed in the build (`tee_add_panel_shaders`, `cmake/TeeShaders.cmake`), and a shader the panel names that the
build does not list fails the build. In a build with no GPU layer, and in every picture the software rasteriser
draws, a shader element is a placeholder that says why. What a shader's code may use (`u_time`, `u_size`,
`u_px`, `in0` to `in15`, `in_<name>`, string literals, `funk/text.sh`, `funk/shapes.sh`) is described in
`ui/kit/shaders/ShaderText.h` and `ShaderLib.h`.

**MIDI learn, the dial menu.** A right-click on a control does nothing: the plugin's menu for it (MIDI learn) is
not here.

**Notes.** What the reader cannot use is a line in `GuiPanel::notes()`, `panel.gui:12:5: note: ...`, and the
rest of the file stands. `#pragma panel` lines in a program (the plugin's older way to lay out dials) are not
read at all.

## What J2 added

*Card J, slice J2: Spec paths, generated pages and widget elements. The reader is still `GuiSpec.cpp`; what the
elements bind to is `ui/kit/bind/Binding.h`. `tests/ui/BindTests.inc` uses each.*

**A Spec path is a name.** `knob boost.level`, `menu engine.layout`, `switch ecu.pops "OFF" "ON"`: a dotted name
(letters, digits and `_` between the dots) is read as one, as `shared->x` and `bands[2]` were. On a `FieldPage`
(a `GuiPanel` bound to a document of Spec fields) the control's parameter is that field, in the user's units; a
display or a widget of a telemetry value names it the same way (`gauge engine.speed`). A path longer than 31
characters is cut (J1's `Control::name`).

**`auto "boost"`** on a line of its own stands for the generated page of the section `boost`: a `section` a
`ui::group` (in the order the fields come; a field with no group under the section's name), a wrapping row of
controls in each (a knob for a number or an integer, a menu for a choice, a switch for a bool), then each table of
the group (`curve`, `map`). A field marked `ui::hidden` is left out. The page is one line of `.gui` text, so the
file's other lines keep their numbers in notes. A section the document does not have is a note.

**Widgets** (`GuiKind::widget`), each `element NAME label="..." width= height=` (and `grow=`, `margin=`, `self=`):

| Element | Names | Is | Cell when unsized |
|---|---|---|---|
| `map PATH` | a `Map16` field | `MapEdit`: a table and a heat map, cells typed or dragged, axes typed, the live cell lit | 560 x 300 |
| `curve PATH` | a `Curve16` field | `CurveEdit`: points dragged on a plot | 320 x 200 |
| `gauge NAME` | a telemetry value | `Gauge`: arc, ticks, numerals, needle, red band (the engine's speed: rpm, its redline) | 170 x 170 |
| `plot NAME` | a telemetry value | `Plot` of its last ten seconds | 320 x 180 |
| `bar NAME` | a telemetry value | `Bar`, in the value's usual range | 150 x 44 |
| `lamp NAME` | a telemetry value | `Lamp`, lit above 0.5 | 64 x 44 |

The reader keeps the element and the name; the panel's owner makes the item (`GuiPanel::Options::widget`; a
`FieldPage` does it with `fieldWidget`). A name nothing answers makes nothing, and the cell stays empty. Widgets
scroll with the interface and are clipped to it; a widget on a page not shown is hidden. A map's live cell is the
engine's operating point, read from the telemetry its axes' quantities suggest (rotation: `engine.speed`; pressure:
`engine.manifold`; fraction: `engine.throttle`), and none while `engine.state` is 0.

**Longer labels.** A control's label may be 18 characters (was 14) and a menu's choice 16 (was 10): a Spec's
labels and choices are a person's words ("Air temperature", "Supercharger").
