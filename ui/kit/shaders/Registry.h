// ui/kit/shaders/Registry.h: the panel shaders the build made (cmake/TeeShaders.cmake). Ours, not from
// TestingPlayground: there the plugin ran shaderc while it played and kept each binary in the program's PanelSpec; here
// shaderc runs when the application is built, the binaries are embedded as arrays, and this is where a panel finds
// them by name.
//
// tee_add_panel_shaders(<target> NAME <panel> ...) gives <target> a function, tee::ui::shaders::<panel>Shaders(), that
// returns the panel's table: each of its shaders' file name and the binary for this platform's renderer (Metal on
// macOS, Direct3D 11 on Windows, SPIR-V elsewhere). Whoever owns the panel adds the table once (add()), before the
// panel is made; GuiPanel then finds each shader by the panel's name and the file's.
//
// In a build with no GPU layer no shader is built, the tables are empty, and a shader element draws FunkGui's
// placeholder (the software rasteriser draws no shader in any build).
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace tee::ui::shaders
{
    struct Built
    {
        const char* panel;                       // the panel's name (tee_add_panel_shaders' NAME)
        const char* file;                        // the shader's file, as the .gui file names it ("cluster.sc")
        const uint8_t* data;                     // shaderc's output for this platform
        uint32_t size;
    };

    // A panel's table, kept by pointer (it is static data). Adding the same table again does nothing.
    void add(std::span<const Built> table);
    // The shader `file` of `panel`, or nullptr.
    const Built* find(std::string_view panel, std::string_view file);
    // Every shader added so far given to FunkGui (funkgui::shaders::set, each under its bytes' hash, with the text
    // library's font bound: ShaderFont.h), so that the first frame compiles nothing. False when FunkGui refused one:
    // `why` says which and what FunkGui said, a line each; the rest are registered all the same. With no GPU layer:
    // true, and nothing done.
    bool registerAll(std::string& why);
    // Why a shader has no binary in this build, for its placeholder's text.
    const char* whyNone();
}
