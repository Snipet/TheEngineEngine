// ui/kit/shaders/shadergen.cpp: tee-shadergen, the build's tool that writes what bgfx's shaderc is handed for a
// panel's shaders (cmake/TeeShaders.cmake runs it, then shaderc). Ours; it calls the text side that came from
// TestingPlayground (ShaderText.h), where the plugin did the same while it ran.
//
//   tee-shadergen --gui panel.gui --prelude fs_custom_prelude.sc --out DIR [--header ink.sh ...] cluster.sc ...
//
// For each shader file listed it writes DIR/<stem>/fs.sc (FunkGui's prelude, the panel's names u_time, u_size, u_px,
// in0..in15 and in_<name> for the inputs= the .gui file gives that shader, then the file's text with its string
// literals made glyph codes) and, beside it, every file it includes: the library's (funk/text.sh, funk/shapes.sh) and
// the --header files, each under its own name. shaderc is then run with -i DIR/<stem>.
//
// A shader the .gui file does not name, or one it names that is not listed, is an error: nothing is built by halves.
// Errors are a line each on stderr, "file:line:column: note: ...", and the exit status is 1.
#include "kit/gui/GuiSpec.h"
#include "kit/shaders/ShaderText.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    bool readFile(const std::filesystem::path& p, std::string& out)
    {
        std::ifstream in(p, std::ios::binary);
        if (!in)
            return false;
        out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        return true;
    }

    bool writeFile(const std::filesystem::path& p, const std::string& text)
    {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        std::ofstream out(p, std::ios::binary | std::ios::trunc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        return static_cast<bool>(out);
    }

    int fail(const std::string& what)
    {
        std::fprintf(stderr, "tee-shadergen: %s\n", what.c_str());
        return 1;
    }
}

int main(int argc, char** argv)
{
    std::filesystem::path guiPath, preludePath, outDir;
    std::vector<std::filesystem::path> headerPaths, shaderPaths;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto value = [&](std::filesystem::path& into) {
            if (i + 1 >= argc)
                return false;
            into = argv[++i];
            return true;
        };
        std::filesystem::path header;
        if (a == "--gui" ? !value(guiPath) : a == "--prelude" ? !value(preludePath) : a == "--out" ? !value(outDir) : false)
            return fail(a + " needs a value");
        if (a == "--header")
        {
            if (!value(header))
                return fail("--header needs a file");
            headerPaths.push_back(header);
        }
        else if (a != "--gui" && a != "--prelude" && a != "--out")
            shaderPaths.emplace_back(a);
    }
    if (guiPath.empty() || preludePath.empty() || outDir.empty())
        return fail("usage: tee-shadergen --gui panel.gui --prelude fs_custom_prelude.sc --out DIR [--header x.sh ...] a.sc ...");

    tee::ui::SourceFile gui{ guiPath.filename().string(), {} };
    std::string prelude;
    if (!readFile(guiPath, gui.text))
        return fail("cannot read " + guiPath.string());
    if (!readFile(preludePath, prelude))
        return fail("cannot read " + preludePath.string());
    tee::ui::Sources headers;
    for (const auto& p : headerPaths)
    {
        tee::ui::SourceFile h{ p.filename().string(), {} };
        if (!readFile(p, h.text))
            return fail("cannot read " + p.string());
        headers.push_back(std::move(h));
    }

    // We have no input, as an instrument has none (GuiPanel reads the file the same way).
    const std::vector<tee::ui::PanelShader> asked = tee::ui::readGuiShaders(gui, true);
    int errors = 0;
    for (const tee::ui::PanelShader& sh : asked)
    {
        bool listed = false;
        for (const auto& p : shaderPaths)
            listed = listed || p.filename().string() == sh.file;
        if (!listed)
        {
            std::fprintf(stderr, "%s:1:1: note: the panel draws the shader \"%s\", which the build does not list\n",
                         gui.name.c_str(), sh.file.c_str());
            ++errors;
        }
    }
    for (const auto& p : shaderPaths)
    {
        const std::string name = p.filename().string();
        const tee::ui::PanelShader* shader = nullptr;
        for (const tee::ui::PanelShader& sh : asked)
            if (sh.file == name && shader == nullptr)
                shader = &sh;
        if (shader == nullptr)
        {
            std::fprintf(stderr, "%s:1:1: note: no shader element of %s names this file\n", name.c_str(), gui.name.c_str());
            ++errors;
            continue;
        }
        std::string body;
        if (!readFile(p, body))
            return fail("cannot read " + p.string());
        const tee::ui::ShaderFiles files = tee::ui::shaderFiles(name, body, shader->inputs, prelude, headers);
        if (!files.error.empty())
        {
            std::fputs(files.error.c_str(), stderr);
            ++errors;
            continue;
        }
        const std::filesystem::path dir = outDir / p.stem();
        bool ok = writeFile(dir / "fs.sc", files.source);
        for (const auto& [included, text] : files.includes)
            ok = ok && writeFile(dir / std::filesystem::path(included), text);
        // Where the shader's own first line is in fs.sc, for whoever reads a line number in shaderc's words.
        ok = ok && writeFile(dir / "bodyline.txt", std::to_string(files.bodyLine) + "\n");
        if (!ok)
            return fail("cannot write under " + dir.string());
    }
    return errors == 0 ? 0 : 1;
}
