// ui/kit/inspect/EditCommand.h: the commands a game controller edits a build with (the owner's card, 26 Nov: "integrate
// some other buttons on [the G29] to make editing engines from the controller possible"). The seam between three
// writers: card B maps the wheel's buttons to these (app/), U1's AppFrame::edit takes them (ui/app/), J2's inspector
// does the most of them (ui/kit/inspect/). In the kit, so the inspector needs nothing of the frame's.
//
// AppFrame::edit(EditCommand, bool fine) -> bool, on the interface's thread (a wheel's thread posts to it): true when
// something took the command. The frame's own: workspacePrev and workspaceNext (the rail's order, GAR ... SCP,
// wrapping at the ends), pagePrev and pageNext (the inspector's pages, stopping at the ends), undo and redo (as the
// UNDO and REDO buttons), back while the frame has something open (the part card, the settings page: closed). Every
// other command goes to the shown page's rows, which are given the focus first when nothing has it (the first press
// lands on a row); false where there are none. While a dialog asks (the key, a question), every command is false.
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
        valueDown,        // the focused value: a step down (`fine`: a tenth of one); a choice the one before; a switch off
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
