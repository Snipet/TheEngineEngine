// tests/sim/core/Presets.h: the layout presets (content/builds/engines/*.json) put into the rig's spec.
#pragma once

#include "Core.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace tee::test::core
{
    inline std::filesystem::path presetDir() { return std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "engines"; }

    inline nlohmann::json loadPreset(const std::string& name)
    {
        const auto text = readFile(presetDir() / (name + ".json"));
        auto doc = nlohmann::json::parse(text, nullptr, false);
        INFO(name);
        REQUIRE(!doc.is_discarded());
        REQUIRE(doc["format"] == "tee-build");
        return doc;
    }

    inline std::vector<std::string> presetNames()
    {
        std::vector<std::string> out;
        for (const auto& entry : std::filesystem::directory_iterator(presetDir()))
            if (entry.path().extension() == ".json")
                out.push_back(entry.path().stem().string());
        std::sort(out.begin(), out.end());
        return out;
    }

    // The preset's engine section into the rig's spec (the layout's fields)
    inline void applyLayout(Rig& rig, const nlohmann::json& doc)
    {
        const auto& e = doc["spec"]["engine"];
        rig.set("spec.custom", e.value("custom", false) ? 1.0 : 0.0);
        rig.set("spec.cylinders", e.value("cylinders", 1));
        rig.set("spec.rotors", e.value("rotors", 0));
        for (const char* name : {"bankAngle", "bank", "pin", "fireRev", "place"})
        {
            const auto& a = e[name];
            for (std::size_t i = 0; i < a.size(); ++i)
                rig.set(std::string("spec.") + name + "[" + std::to_string(i) + "]", a[i].get<double>());
        }
    }
} // namespace tee::test::core
