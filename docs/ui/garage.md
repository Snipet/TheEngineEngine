# GARAGE: the builds

*Package U5. Code: `ui/views/garage/`. Tests: `tests/ui/views/GarageTests.cpp`. Pictures: the software rasteriser's.*

**The cards.** Every build and part in the library: its name; what it is (WHOLE BUILD, ENGINE, CAR, TUNE, in a frame);
SHIPPED, YOURS or CURRENT (the current also has an accent bar down its edge, and `* UNSAVED` when the document differs);
the engine in a sentence ("3.0 L straight six, twin turbo, E85", `Describe.h`), or for a car or tune its first values;
peak torque and power with their speeds, in the user's units; a small power curve on a scale common to every card. A
search field (name, sentence, notes; filters as it is typed), SORT by name, power, displacement or layout (cylinders),
SHOW all, builds, engines, cars or tunes. The wheel scrolls the cards (eased).

**The card's column** (the stage's right; it stands in for an inspector, since the frame's inspector shows only Spec
fields): its name, part, sentence and notes, and what can be done with it:
- another whole build: LOAD THIS BUILD; ITS ENGINE (into this car), ITS CAR (under this engine), ITS TUNE (on this
  engine); COMPARE; DUPLICATE; DELETE (yours only, then CONFIRM DELETE or KEEP IT);
- a part alone: PUT THIS ENGINE IN, USE THIS CAR or TRY THIS TUNE;
- the current build: SAVE (yours) and REVERT, SAVE AS (a name typed), NOTES (typed), DUPLICATE.
Each load is one undo step. What the last action did is said over the buttons (green bar; amber with the reason).

**Compare**: torque (heavy) and power (light) of the current build (solid, the ink) and the other (dashed, the accent)
on one plot with a legend, and every field that differs, grouped by section, both values in the user's units.

## The library seam

`Library.h` is what GARAGE needs: `builds()` (summaries: id, name, notes, part, user, current, values by path in SI,
predicted curve), `revision()`, `tick()`, `load(id, part)`, `save`, `saveAs(name)`, `duplicate`, `remove`, `revert`,
`unsaved`, `notes`/`setNotes`. The view never touches the file system. Implementations:
- `DemoLibrary`: ten made-up builds and parts in memory, for tests and pictures; the stage then says DEMO LIBRARY.
- `ServicesLibrary`: over the frame's `FrameServices` (names, load, save as) as they are today: whole builds only, no
  curves but the current one's.
- Wanted: `WorkspaceContext::library` (U1) and an implementation over `tee::Build` and `tee::Presets` in `shell/files/`
  (card B); `docs/handoffs/u5-garage-scope.md` says exactly what.

Tests: `garage.stage` (+`.dpi1`), `garage.large` (+`.dpi1`, `garage.paper`), `garage.compare` (+`.dpi1`),
`garage.search`, `garage.parts`, `garage.files`, `garage.rest`.
