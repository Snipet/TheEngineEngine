// ui/kit/shaders/Registry.cpp: see Registry.h.
#include "kit/shaders/Registry.h"

#include "kit/shaders/ShaderFont.h"
#include "kit/shaders/ShaderText.h"

#include <funkgui/gpu/ShaderLibrary.h>

#include <algorithm>
#include <vector>

namespace tee::ui::shaders
{
    namespace
    {
        std::vector<std::span<const Built>>& tables()
        {
            static std::vector<std::span<const Built>> all;
            return all;
        }
    }

    void add(std::span<const Built> table)
    {
        auto& all = tables();
        const bool have = std::any_of(all.begin(), all.end(), [&](const auto& t) { return t.data() == table.data(); });
        if (!have && !table.empty())
            all.push_back(table);
    }

    const Built* find(std::string_view panel, std::string_view file)
    {
        for (const auto& table : tables())
            for (const Built& b : table)
                if (panel == b.panel && file == b.file && b.size > 0)
                    return &b;
        return nullptr;
    }

    bool registerAll(std::string& why)
    {
        bool ok = true;
#if TEE_UI_GPU
        for (const auto& table : tables())
            for (const Built& b : table)
            {
                if (b.size == 0)
                    continue;
                const std::vector<uint8_t> bytes(b.data, b.data + b.size);
                const uint32_t id = shaderId(bytes);
                std::string refused;
                if (funkgui::shaders::has(id) || funkgui::shaders::set(id, b.data, b.size, refused))
                    bindShaderFont(id);
                else
                {
                    ok = false;
                    why += std::string(b.panel) + "/" + b.file + ": " + (refused.empty() ? "the GPU refused it" : refused) + "\n";
                }
            }
#else
        (void)why;
#endif
        return ok;
    }

    const char* whyNone()
    {
#if TEE_UI_GPU
        return "this build has no binary for this shader: it was not listed when the application was built";
#else
        return "this build has no GPU renderer: shaders draw as placeholders";
#endif
    }
}
