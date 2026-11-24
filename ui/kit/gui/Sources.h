// ui/kit/gui/Sources.h: from TestingPlayground, Source/dsp/Sources.h at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// Sources.h: a panel's files by name: its .gui file, the shaders it names (.sc) and what they include (.sh). Cut down
// from the plugin's list of a program's files (its editor's tabs) to the struct and the lookup the panel code uses.
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tee::ui
{
    struct SourceFile
    {
        std::string name;
        std::string text;

        friend bool operator==(const SourceFile&, const SourceFile&) = default;
    };

    using Sources = std::vector<SourceFile>;

    // The file named `name`, or nullptr.
    inline const SourceFile* findFile(const Sources& files, std::string_view name)
    {
        for (const SourceFile& f : files)
            if (f.name == name)
                return &f;
        return nullptr;
    }
}
