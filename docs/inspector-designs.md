# The inspector: three designs for the owner to choose from

*J2, 25 Nov. The owner: "Are there any plans for redesigning the right-side controls on the views? I feel like they
are relatively clunky still." Nothing of this is in the application: the shipped inspector is unchanged until one is
chosen. Every picture is drawn on Linux by FunkGui's software rasteriser, over the real Spec in the real frame.*

**Pictures**: `tee-ui-build-tests build.inspect.pictures` (root build) writes `out/ui/inspect/`. Each design at the
smallest window (`.small`, 1100 x 700) and a large one (`.large`, 1600 x 1000), on ENGINE 1 (`engine`), TUNE's FUEL
page (`fuel`), EXHAUST 1 (`exhaust`, and `.scrolled`) and TUNE's ECU 1, a page of choices and switches (`choices`).

## What all three change

Today a value is a synthesiser's knob: a 200 px drag for the whole range, a wheel notch a hundredth, and no way to
type a number. In all three:

- **A value is typed.** Click it and type "86", "86 mm", "3.39 in" or "1 bar"; a choice by its name. Outside the
  field's range it is **refused, never clamped**, and the range is said: "Out of range: 0.05 to 30.00 L"
  (`rows.engine.*.refused`).
- A drag sideways scrubs it, the wheel nudges it, a double-click is its default; each edit is one undo step.
- The keyboard: one Tab stop for the column, Up and Down between fields, a digit types, Left and Right nudge.
- Hover help stays, each row's own, with its range and default.

**Chosen (25 Nov, the owner: "As recommended")**: Rows goes into the application (`RowsPage`, the frame's hook:
`docs/ui-kit.md` "Rows in the frame"), the card on the picture is added to it (`PartCard`, "The card on the picture:
the second hook"), and Dials is dropped. The pictures below are the design harness's, before the frame's hook.

## The three

1. **Rows** (`rows.*`). One field a row: its name, its value and unit, and a thin bar of where it sits in its range
   with a tick at its default. A dot marks every field off its default. Groups fold. A filter box finds a field by
   name in every page of the workspace (`rows.filter.*`). CHANGED shows only what moved (`rows.changed.*`).
2. **Dials, made exact** (`dials.*`). A larger dial for each number, its value typed under it; menus and switches as
   cells; a steady grid of two columns; a group's heading stays at the top while the group scrolls under it.
3. **On the picture** (`picture.engine.*`, `picture.exhaust.*`, `picture.tune.*`). Press a part on the stage (a
   cylinder, the H-pipe) and a card opens beside it with its first six fields; the column is a compact, searchable
   list of everything in the workspace.

## What each costs to build

All three read every workspace's existing page, so **no workspace has to change** for 1 or 2.

| | Build | Keyboard | Hover help |
|---|---|---|---|
| 1. Rows | Built. One hook in the frame (U1) to show it in place of today's page; tables keep their own pages. | Best: arrows and typing. | Per row. |
| 2. Dials | Built; the same hook. Long pages get long: EXHAUST 1 is about 22 rows of dials. | As Rows. | Per cell. |
| 3. On the picture | The list is Rows' (free). The card works today wherever a part is pressed. To open it from the list or the keyboard, each of the eleven workspaces says where its parts are (about 20 lines each, its own package's). | The card is a second Tab stop. | Per row. |

## The pick: Rows now, the card next

Rows fixes what the owner feels most (exact numbers, typed, refused out of range) at the least cost, and works on
every workspace the day the frame's hook lands. The card (design 3) builds on Rows' list and is worth adding once
each workspace can say where its parts are. Dials keep a synthesiser's look that this workshop does not need.
