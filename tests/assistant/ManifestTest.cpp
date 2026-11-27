// tests/assistant/ManifestTest.cpp: the Claude Desktop extension's manifest (assistant/CMakeLists.txt configures it into
// <build>/mcpb/manifest.json, which the target tee-mcpb packs) as Claude Desktop reads it: valid JSON, and the fields
// the MCPB manifest reference requires present and of their kind. A manifest that does not parse is an extension Claude
// Desktop refuses, which only a real Claude Desktop would otherwise find.
#include <nlohmann/json.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <sstream>
#include <string>

#if defined(TEE_MCPB_MANIFEST)

TEST_CASE("assistant: the extension's manifest is JSON Claude Desktop can read, with its required fields", "[assistant]")
{
    std::ifstream in(TEE_MCPB_MANIFEST, std::ios::binary);
    REQUIRE(in.good());
    std::stringstream text;
    text << in.rdbuf();
    const nlohmann::json m = nlohmann::json::parse(text.str(), nullptr, false);
    REQUIRE_FALSE(m.is_discarded());   // a parse error: the extension would be refused
    REQUIRE(m.is_object());
    for (const char* key : {"manifest_version", "name", "version", "description", "author", "server"})
    {
        INFO(key);
        CHECK(m.contains(key));
    }
    CHECK(m["manifest_version"].is_string());
    CHECK(m["name"].is_string());
    CHECK(m["version"].is_string());
    CHECK(m["author"].contains("name"));
    const nlohmann::json& s = m["server"];
    CHECK(s["type"] == "binary");
    CHECK(s["entry_point"] == "server/tee-mcp");
    CHECK(s["mcp_config"]["command"] == "${__dirname}/server/tee-mcp");
    CHECK(s["mcp_config"]["args"].is_array());
    REQUIRE(m.contains("compatibility"));
    REQUIRE(m["compatibility"]["platforms"].is_array());
    for (const auto& p : m["compatibility"]["platforms"])
        CHECK((p == "darwin" || p == "linux"));
    for (const auto& t : m["tools"])
    {
        CHECK(t["name"].is_string());
        CHECK(t["description"].is_string());
    }
    // what the owner is told is what the app draws: its own Undo and its status line, no card (none is drawn yet)
    const std::string said = m.value("description", std::string()) + " " + m.value("long_description", std::string());
    INFO(said);
    CHECK(said.find("card") == std::string::npos);
    CHECK(said.find("Undo") != std::string::npos);
}

#endif
