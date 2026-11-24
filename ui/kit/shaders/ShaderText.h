// ui/kit/shaders/ShaderText.h: from TestingPlayground, Source/fminus/Shaders.h at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// ShaderText.h: a .gui file's shaders (GuiSpec.h: shader "glow.sc" inputs="cutoff env out") made ready for bgfx's
// shaderc (FunkGui-free: the build's tool, kit/shaders/shadergen.cpp, and the panel both use it). A shader's file, one of the program's, holds the body of a fragment shader in bgfx's
// language (GLSL with bgfx's vec types and functions): helper functions and a main() that sets gl_FragColor, straight
// RGBA. shaderSource() puts FunkGui's head before it (funkgui/gpu/ShaderLibrary.h: $input v_uv, v_px, bgfx_shader.sh,
// u_viewSize, u_rect, u_params) and the panel's names:
//
//   u_time      float  seconds, the editor's clock (it moves while the panel draws)
//   u_size      vec2   the element's size in px
//   u_px        float  one of the screen's pixels in px (0.5 on a Retina screen): the width to smooth an edge over
//   in0 .. in15 float  its inputs in order (kMaxShaderInputs: FunkGui's 16 params); in_<name> the same by the
//                      input's name (in_cutoff, in_env, in_out):
//                      a parameter's value 0..1 (its knob's turn), a field's value as the program writes it, in and
//                      out the audio's level (its peak, 0..1)
// and v_uv (0..1 across, y down) and v_px (px from the top left corner), FunkGui's varyings.
//
// Text and includes (shaderText, shaderIncludes): before shaderc, the file's string literals become glyph codes
// ("RPM" a vec4 of them, a longer one a mat4: fminus/ShaderLib.h), and its #include lines are resolved: the built-in
// library (funk/text.sh, funk/shapes.sh: ShaderLib.h) and the program's own .sh files (ShaderJob::headers), in turn
// what they include, each once (a file's text wrapped in an include guard, as FMinus reads a file once), written beside
// the shader for shaderc's -i. A string that cannot be one (too long, a character the font lacks, no end on its line)
// or an #include of a file there is none of is a note on its line, and nothing is built. A note shaderc gives is cited
// at the line of the shader or of the program's .sh file that has what it names; text.sh's font is a texture
// (editor/ShaderFont.h binds it to every shader's s_funkFont).
//
// Here the shaders are built when the application is built (cmake/TeeShaders.cmake), never while it runs: this file is
// the text side alone. shaderFiles() gives what shaderc is handed (the fragment source, and the files it includes,
// written beside it for shaderc's -i); shaderc's own errors are the build's, in shaderc's words, with ShaderFiles::
// bodyLine to find the shader's line by. The plugin this came from ran shaderc itself, with a cache on disk and a
// check by the renderer: all of that is cut.
#pragma once

#include "kit/gui/Sources.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace tee::ui
{
    // The program's files a shader may #include: its .sh files.
    Sources shaderHeaders(const Sources& files);

    // A shader's file (or a .sh file's) as shaderc reads it: every string literal outside comments and #include lines
    // made its glyph codes (fminus/ShaderLib.h: up to 12 characters a vec4, up to 48 a mat4; a backslash before a
    // backslash, a quote, or a u and four hex digits (a code point) escapes it), and every vector made of one value
    // (vec3(x), as GLSL writes x three times or x's first three) a cast, ((vec3)(x)), which Direct3D's compiler takes as
    // Metal's and SPIR-V's do (it takes float3(x) only of three); the rest as written, line for line. `error`: the
    // first string that cannot be one, as a note on its line and column ("" when none).
    std::string shaderText(const std::string& file, const std::string& text, std::string& error);

    // The files a shader's text #includes (outside comments), and in turn theirs, each once, in the order they are
    // reached: the library's (funk/text.sh) and `headers`' names. `error`: the first #include of a file that is
    // neither, as a note on its line ("" when none). bgfx_shader.sh, which every shader includes, is not listed.
    std::vector<std::string> shaderIncludes(const std::string& file, const std::string& body, const Sources& headers,
                                            std::string& error);

    // The fragment shader shaderc compiles: `prelude` (FunkGui's head: funkgui::shaders::fragmentPrelude()'s
    // text), the panel's names for `inputs` (up to sixteen names), and
    // the file's text. bodyLine: the line of the result where the file's first line is.
    std::string shaderSource(const std::string& body, const std::vector<std::string>& inputs, const std::string& prelude,
                             int* bodyLine = nullptr);

    // What shaderc is handed for a shader: `source` (shaderSource of shaderText(body)), and the files it includes
    // (shaderIncludes: the library's, and `headers`' each through shaderText and wrapped in an include guard), by the
    // names to write them under beside it. `error`: the pre-pass's note, when the shader cannot be built ("" when it
    // can). bodyLine: the line of `source` where the file's first line is.
    struct ShaderFiles
    {
        std::string source;
        std::vector<std::pair<std::string, std::string>> includes;
        std::string error;
        int bodyLine = 1;
    };
    ShaderFiles shaderFiles(const std::string& file, const std::string& body, const std::vector<std::string>& inputs,
                            const std::string& prelude, const Sources& headers = {});

    // A shader binary's id for FunkGui's ShaderLibrary: its bytes' hash, never 0 (the same shader, the same id).
    uint32_t shaderId(const std::vector<uint8_t>& binary);

    // Whether a shader's file reads the editor's clock: u_time (or u_viewSize, whose w it is) as a word of its code,
    // never in a comment (// to the line's end, a backslash at its end going on, and /* */) or a string, nor part of a
    // longer name or a number. Such a shader changes every frame (the Panel draws it at full rate); any other only when
    // an input does.
    bool shaderAnimates(const std::string& body);
    // The same of a shader and the program's .sh files it includes (shaderIncludes; the library reads no clock).
    bool shaderAnimates(const std::string& file, const std::string& body, const Sources& headers);
}
