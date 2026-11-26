// app/WheelEdit.h: the wheel's edit commands (shell/input/g29/G29Input.h's EditCommand: the shell's core reaches no
// ui/) made the interface's (ui/kit/gui/EditCommand.h, the seam U1's frame and J2's inspector serve), by name: "rowup"
// is rowUp. Where the interface's frame is built (TEE_HAS_UI_FRAME) only.
#pragma once

#include "input/g29/G29Input.h"
#include "kit/gui/EditCommand.h"

#include <cctype>
#include <optional>
#include <string_view>

namespace tee::app
{
    static_assert(static_cast<int>(input::g29::EditCommand::count) == ui::kEditCommands,
                  "the wheel's edit commands and the interface's are one list");

    inline std::optional<ui::EditCommand> uiEdit(input::g29::EditCommand c) noexcept
    {
        const std::string_view mine = input::g29::editName(c);
        for (int i = 0; i < ui::kEditCommands; ++i)
        {
            const std::string_view theirs = ui::editCommandName(static_cast<ui::EditCommand>(i));
            bool same = theirs.size() == mine.size();
            for (std::size_t k = 0; same && k < mine.size(); ++k)
                same = std::tolower(static_cast<unsigned char>(theirs[k])) == static_cast<unsigned char>(mine[k]);
            if (same)
                return static_cast<ui::EditCommand>(i);
        }
        return std::nullopt;
    }
}
