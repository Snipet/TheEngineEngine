// ui/kit/gui/EditCommand.h: the commands a game controller edits a build with (the owner's card, 26 Nov: "integrate
// some other buttons on [the G29] to make editing engines from the controller possible"). The seam between three
// writers: card B maps the wheel's buttons to these (app/), U1's AppFrame::edit takes them (ui/app/), J2's inspector
// does the most of them (ui/kit/inspect/). Beside Item (Items.h), which takes them: Item::edit(EditCommand, bool fine),
// false by default.
//
// AppFrame::edit(EditCommand, bool fine) -> bool, on the interface's thread (a wheel's thread posts to it): true when
// something took the command. Every command goes to the focused item first (the rows, a table: pagePrev and pageNext
// move a table's cell); the frame does what comes back false: workspacePrev and workspaceNext (the rail's order,
// wrapping at the ends), pagePrev and pageNext (the inspector's pages, stopping at the ends; the new page's rows given
// the focus), undo and redo (as the UNDO and REDO buttons), back (the part card, the chat, the settings closed; from a
// table's page, back to the rows it was opened from). With nothing focused, the shown page's rows are given the focus
// first (the first press lands on a row). While a dialog asks (the key, a question), every command is false. `fine`:
// a tenth of a step, for the coarse nudges too.
//
// The default map (card B's, so the owner's hands know): D-pad up and down rows; left and right the inspector's pages;
// L2 and R2 the workspaces; the red dial a nudge of the focused value (SHARE held: fine); minus and plus a coarse
// nudge; the dial's press and cross activate; circle back; square undo; triangle redo; L3 the default; OPTIONS the
// "changed only" filter.
#pragma once

namespace tee::ui
{
    enum class EditCommand
    {
        rowUp,            // the focused row: the one above (a heading's row too)
        rowDown,
        pagePrev,         // the inspector's pages (the frame's)
        pageNext,
        workspacePrev,    // the rail's workspaces (the frame's)
        workspaceNext,
        valueDown,        // the focused value: a step down (`fine`: a tenth of one, here and coarse); a choice the one before
        valueUp,
        valueDownCoarse,  // ten steps
        valueUpCoarse,
        activate,         // the focused row: a table's OPEN, a choice's menu, a heading folded or unfolded
        back,             // what is open closes (a menu, the typing, the filter; the frame's card and settings first)
        undo,             // the frame's
        redo,
        toDefault,        // the focused field to its default (one undo step)
        changedOnly,      // the "changed only" filter on or off
    };

    inline constexpr int kEditCommands = static_cast<int>(EditCommand::changedOnly) + 1;

    // Its name as the enumerator's ("rowUp"), for a map's file and the logs; "" for none.
    constexpr const char* editCommandName(EditCommand c) noexcept
    {
        switch (c)
        {
            case EditCommand::rowUp: return "rowUp";
            case EditCommand::rowDown: return "rowDown";
            case EditCommand::pagePrev: return "pagePrev";
            case EditCommand::pageNext: return "pageNext";
            case EditCommand::workspacePrev: return "workspacePrev";
            case EditCommand::workspaceNext: return "workspaceNext";
            case EditCommand::valueDown: return "valueDown";
            case EditCommand::valueUp: return "valueUp";
            case EditCommand::valueDownCoarse: return "valueDownCoarse";
            case EditCommand::valueUpCoarse: return "valueUpCoarse";
            case EditCommand::activate: return "activate";
            case EditCommand::back: return "back";
            case EditCommand::undo: return "undo";
            case EditCommand::redo: return "redo";
            case EditCommand::toDefault: return "toDefault";
            case EditCommand::changedOnly: return "changedOnly";
        }
        return "";
    }
}
