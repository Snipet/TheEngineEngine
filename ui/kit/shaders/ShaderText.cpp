// ui/kit/shaders/ShaderText.cpp: from TestingPlayground, Source/fminus/Shaders.cpp at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// ShaderText.cpp: see ShaderText.h.
#include "kit/shaders/ShaderText.h"
#include "kit/gui/PanelSpec.h"
#include "kit/shaders/ShaderLib.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace tee::ui
{
    namespace
    {
        uint64_t fnv64(std::string_view bytes, uint64_t h = 14695981039346656037ull)
        {
            for (const char c : bytes)
                h = (h ^ static_cast<unsigned char>(c)) * 1099511628211ull;
            return h;
        }

        std::string hex64(uint64_t v)
        {
            char buf[17];
            std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(v));
            return buf;
        }

        // An input's name as a shader name: letters, digits and _ ("in_cutoff").
        std::string inputName(const std::string& name)
        {
            std::string out;
            for (const char c : name)
                out += std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
            return out;
        }


        // ---- text and includes (Shaders.h) ---------------------------------------------------------------------------
        // A note of the pre-pass's: the shader does not compile, for `why`, at a line and column of `file`.
        std::string prepassNote(const std::string& file, int line, int column, const std::string& why)
        {
            return file + ":" + std::to_string(line) + ":" + std::to_string(column) + ": note: the shader does not compile: "
                 + why + " (it draws nothing until it does)\n";
        }

        // Walks a shader's text as shaderc's preprocessor reads it: its comments (// to the line's end, a backslash at
        // its end going on; /* */), its #include lines and the rest. `include` hears each #include's file name and
        // place; `string` each string literal's place and inner text (between its quotes, as written), and returns
        // what replaces the literal. The text, each literal replaced; a literal with no end on its line goes to
        // `string` with `ended` false.
        template <typename Include, typename String>
        std::string walk(const std::string& text, Include&& include, String&& string)
        {
            std::string out;
            out.reserve(text.size());
            const std::size_t n = text.size();
            std::size_t i = 0, lineStart = 0;
            int line = 1;
            bool blank = true;                       // only spaces since the line's start (a directive may begin)
            const auto newline = [&](std::size_t at) {
                ++line;
                lineStart = at + 1;
                blank = true;
            };
            while (i < n)
            {
                const char c = text[i];
                if (c == '\n')
                {
                    out += c;
                    newline(i);
                    ++i;
                    continue;
                }
                if (c == '/' && i + 1 < n && text[i + 1] == '/')
                {
                    for (; i < n && text[i] != '\n'; ++i)
                    {
                        out += text[i];
                        if (text[i] == '\\' && i + 1 < n && text[i + 1] == '\n')
                        {
                            out += '\n';
                            newline(i + 1);
                            ++i;
                        }
                    }
                    continue;
                }
                if (c == '/' && i + 1 < n && text[i + 1] == '*')
                {
                    const std::size_t end = text.find("*/", i + 2);
                    const std::size_t stop = end == std::string::npos ? n : end + 2;
                    for (; i < stop; ++i)
                    {
                        out += text[i];
                        if (text[i] == '\n')
                            newline(i);
                    }
                    blank = false;
                    continue;
                }
                if (c == '#' && blank)
                {
                    // A directive: an #include's name heard and its line copied as it is.
                    std::size_t k = i + 1;
                    while (k < n && (text[k] == ' ' || text[k] == '\t'))
                        ++k;
                    if (text.compare(k, 7, "include") == 0 && (k + 7 >= n || !std::isalnum(static_cast<unsigned char>(text[k + 7]))))
                    {
                        std::size_t e = text.find('\n', k);
                        if (e == std::string::npos)
                            e = n;
                        std::size_t q = k + 7;
                        while (q < e && (text[q] == ' ' || text[q] == '\t'))
                            ++q;
                        if (q < e && (text[q] == '"' || text[q] == '<'))
                        {
                            const char close = text[q] == '"' ? '"' : '>';
                            // Looked for on its line alone: a search to the text's end, line after line, would make
                            // a file of many unclosed #include lines quadratic.
                            const std::size_t z = std::string_view(text.data(), e).find(close, q + 1);
                            if (z != std::string::npos)
                                include(text.substr(q + 1, z - q - 1), line, static_cast<int>(q - lineStart) + 1);
                        }
                        out.append(text, i, e - i);
                        i = e;
                        continue;
                    }
                }
                if (c == '"')
                {
                    std::size_t k = i + 1;
                    while (k < n && text[k] != '"' && text[k] != '\n')
                        k += text[k] == '\\' && k + 1 < n && text[k + 1] != '\n' ? 2 : 1;
                    const bool ended = k < n && text[k] == '"';
                    out += string(std::string_view(text).substr(i + 1, k - i - 1), line, static_cast<int>(i - lineStart) + 1, ended);
                    i = ended ? k + 1 : k;
                    blank = false;
                    continue;
                }
                if (c != ' ' && c != '\t' && c != '\r')
                    blank = false;
                out += c;
                ++i;
            }
            return out;
        }

        // A string literal's glyph codes (ShaderLib.h), from its inner text; false, with why, when one cannot be.
        bool codesOf(std::string_view s, std::vector<int>& codes, std::string& why)
        {
            for (std::size_t i = 0; i < s.size();)
            {
                uint32_t cp = static_cast<unsigned char>(s[i]);
                std::size_t len = 1;
                if (cp == '\\')
                {
                    const char e = i + 1 < s.size() ? s[i + 1] : '\0';
                    if (e == '\\' || e == '"')
                    {
                        cp = static_cast<uint32_t>(e);
                        len = 2;
                    }
                    else if (e == 'u' && i + 6 <= s.size()
                             && std::all_of(s.begin() + static_cast<std::ptrdiff_t>(i + 2), s.begin() + static_cast<std::ptrdiff_t>(i + 6),
                                            [](char h) { return std::isxdigit(static_cast<unsigned char>(h)) != 0; }))
                    {
                        cp = static_cast<uint32_t>(std::strtoul(std::string(s.substr(i + 2, 4)).c_str(), nullptr, 16));
                        len = 6;
                    }
                    else
                    {
                        why = "a string's backslash escapes a backslash, a quote or a code point (a u and four hex digits), "
                              "not this";
                        return false;
                    }
                }
                else if (cp >= 0x80)
                {
                    // UTF-8
                    const int more = cp >= 0xF0 ? 3 : cp >= 0xE0 ? 2 : cp >= 0xC0 ? 1 : -1;
                    if (more < 0 || i + static_cast<std::size_t>(more) >= s.size())
                    {
                        why = "a string holds a byte that is no character";
                        return false;
                    }
                    cp &= more == 3 ? 0x07u : more == 2 ? 0x0Fu : 0x1Fu;
                    for (int k = 1; k <= more; ++k)
                    {
                        const auto b = static_cast<unsigned char>(s[i + static_cast<std::size_t>(k)]);
                        if ((b & 0xC0u) != 0x80u)
                        {
                            why = "a string holds a byte that is no character";
                            return false;
                        }
                        cp = (cp << 6) | (b & 0x3Fu);
                    }
                    len = static_cast<std::size_t>(more) + 1;
                }
                const int code = shaderlib::glyphCode(cp);
                if (code < 0)
                {
                    char hex[16];
                    std::snprintf(hex, sizeof hex, "U+%04X", static_cast<unsigned>(cp));
                    why = std::string("the font has no ") + hex + ": a string holds printable ASCII, and these by their "
                          "codes: \\u00B0 for the degree sign; \\u00B1, \\u00D7, \\u00B5, \\u2212 (minus), \\u2026, \\u2264, \\u2265, "
                          "\\u2248, \\u0394, \\u2013, \\u2014, \\u2022, \\u00B7, \\u2190 to \\u2193 (arrows), \\u221E";
                    return false;
                }
                codes.push_back(code);
                i += len;
            }
            if (codes.size() > static_cast<std::size_t>(shaderlib::kMaxString))
            {
                why = "a string holds " + std::to_string(shaderlib::kMaxString) + " characters at most (this one "
                    + std::to_string(codes.size()) + ")";
                return false;
            }
            return true;
        }

        // The codes as a vec4 (up to 12) or a mat4 (up to 48), three a float, the first the lowest byte.
        std::string packed(const std::vector<int>& codes)
        {
            const std::size_t floats = codes.size() <= 12 ? 4 : 16;
            std::vector<uint32_t> v(floats, 0u);
            for (std::size_t k = 0; k < codes.size(); ++k)
                v[k / 3] |= static_cast<uint32_t>(codes[k]) << (8 * (k % 3));
            std::string out = floats == 4 ? "vec4(" : "mat4(";
            for (std::size_t k = 0; k < floats; ++k)
                out += (k == 0 ? "" : ", ") + std::to_string(v[k]) + ".0";
            return out + ")";
        }

        // A file's include guard's name, from its name.
        std::string guardOf(const std::string& name)
        {
            return "TEE_INCLUDED_" + hex64(fnv64(name));
        }

        // ---- a vector of one value (shaderText) -------------------------------------------------------------------
        // shaderc reads a shader as HLSL, vec3 being float3 (its own #defines). glslang, which compiles it for Metal
        // and SPIR-V, takes float3(x) of one value as GLSL takes vec3(x) (x three times, or x's first three); Direct3D's
        // compiler does not. A cast, ((vec3)(x)), is the same to all three. So a vector type's constructor of one value
        // (vec2 to vec4, and the ivec, uvec and bvec ones) is written as a cast, at any depth; one of more values,
        // comments and the rest are left as they are. Characters are only added, within a line: every line keeps its
        // number.
        bool identChar(char c)
        {
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        }

        bool vectorType(std::string_view w)
        {
            if (w.size() == 5 && (w[0] == 'i' || w[0] == 'u' || w[0] == 'b'))
                w.remove_prefix(1);
            return w.size() == 4 && w.substr(0, 3) == "vec" && w[3] >= '2' && w[3] <= '4';
        }

        // Past the comment that starts at i (before `to`), or i when none does there.
        std::size_t pastComment(const std::string& t, std::size_t i, std::size_t to)
        {
            if (i + 1 >= to || t[i] != '/' || (t[i + 1] != '/' && t[i + 1] != '*'))
                return i;
            const std::size_t e = t[i + 1] == '/' ? t.find('\n', i) : t.find("*/", i + 2);
            const std::size_t end = e == std::string::npos ? to : (t[i + 1] == '/' ? e : e + 2);
            return std::min(end, to);
        }

        // The text's brackets in one pass: each '(' outside a comment with its ')' (npos: none), whether a comma stands
        // between them outside a nested ( ), [ ] or { } (the brackets counted as a depth, any closer taking one off, as
        // a C reader that does not check their kinds), and whether anything but space and comments does. Stacks, never
        // a recursion, and each character looked at once (a comma marks each paren once): a file's depth or length never
        // drives the stack or the time.
        struct Paren
        {
            std::size_t open = 0, close = std::string::npos;
            bool comma = false, something = false;
        };

        std::vector<Paren> parensOf(const std::string& t)
        {
            struct Level
            {
                std::vector<std::size_t> open;       // the parens not closed yet opened at this depth, innermost last
                std::size_t marked = 0;              // the first `marked` of them have a comma already
            };
            std::vector<Paren> parens;
            std::vector<std::size_t> open;           // every paren not closed yet, innermost last (indexes into parens)
            std::vector<std::ptrdiff_t> openDepth;   // the depth inside each
            std::unordered_map<std::ptrdiff_t, Level> levels;
            std::ptrdiff_t depth = 0;
            for (std::size_t i = 0; i < t.size();)
            {
                if (const std::size_t past = pastComment(t, i, t.size()); past != i)
                {
                    i = past;
                    continue;
                }
                const char c = t[i];
                const bool space = c == ' ' || c == '\t' || c == '\r' || c == '\n';
                if (!space && c != ')' && !open.empty())
                    parens[open.back()].something = true;   // inside the innermost paren, so inside each around it
                if (c == '(' || c == '[' || c == '{')
                {
                    ++depth;
                    if (c == '(')
                    {
                        open.push_back(parens.size());
                        openDepth.push_back(depth);
                        levels[depth].open.push_back(parens.size());
                        parens.push_back({ i, std::string::npos, false, false });
                    }
                }
                else if (c == ')' || c == ']' || c == '}')
                {
                    --depth;
                    if (c == ')' && !open.empty())
                    {
                        const std::size_t p = open.back();
                        Level& at = levels[openDepth.back()];
                        open.pop_back();
                        openDepth.pop_back();
                        at.open.pop_back();          // the innermost paren is the last opened at its depth too
                        at.marked = std::min(at.marked, at.open.size());
                        parens[p].close = i;
                        if (!open.empty() && parens[p].something)
                            parens[open.back()].something = true;
                    }
                }
                else if (c == ',')
                    if (const auto it = levels.find(depth); it != levels.end())
                    {
                        Level& at = it->second;      // each paren open where this comma is at its own depth
                        for (; at.marked < at.open.size(); ++at.marked)
                            parens[at.open[at.marked]].comma = true;
                    }
                ++i;
            }
            return parens;
        }

        // `t` with each vector of one value a cast: one pass over parensOf's answers, so linear in the text whatever its
        // nesting.
        std::string castOnes(const std::string& t)
        {
            const std::vector<Paren> parens = parensOf(t);
            std::string out;
            out.reserve(t.size() + t.size() / 16);
            std::vector<std::size_t> ends;           // the ')' of each cast still open, innermost last: each gets one more
            std::size_t i = 0;
            while (i < t.size())
            {
                if (const std::size_t past = pastComment(t, i, t.size()); past != i)
                {
                    out.append(t, i, past - i);
                    i = past;
                    continue;
                }
                if (!ends.empty() && i == ends.back())
                {
                    out += "))";
                    ends.pop_back();
                    ++i;
                    continue;
                }
                if (!identChar(t[i]))
                {
                    out += t[i++];
                    continue;
                }
                std::size_t e = i;                   // a whole word (a name, or a number)
                while (e < t.size() && identChar(t[e]))
                    ++e;
                const std::string_view word(t.data() + i, e - i);
                std::size_t open = e;
                while (open < t.size() && (t[open] == ' ' || t[open] == '\t' || t[open] == '\r' || t[open] == '\n'))
                    ++open;
                const Paren* p = nullptr;
                if (vectorType(word) && open < t.size() && t[open] == '(')
                {
                    const auto it = std::lower_bound(parens.begin(), parens.end(), open,
                                                     [](const Paren& q, std::size_t at) { return q.open < at; });
                    if (it != parens.end() && it->open == open && it->close != std::string::npos)
                        p = &*it;
                }
                if (p != nullptr && !p->comma && p->something)
                {
                    out += "((";
                    out.append(word);
                    out += ")";
                    ends.push_back(p->close);
                }
                else
                    out.append(word);
                i = e;
            }
            return out;
        }

    }

    std::string shaderSource(const std::string& body, const std::vector<std::string>& inputs, const std::string& prelude,
                             int* bodyLine)
    {
        std::string s = prelude;
        if (!s.empty() && s.back() != '\n')
            s += '\n';
        s += "// The panel's names (kit/shaders/ShaderText.h).\n"
             "#define u_time (u_viewSize.w)\n"
             "#define u_size (u_rect.zw)\n"
             "#define u_px (u_viewSize.z)\n";
        static constexpr const char* kSwizzle[4] = { "x", "y", "z", "w" };
        for (int k = 0; k < kMaxShaderInputs; ++k)
            s += "#define in" + std::to_string(k) + " (u_params[" + std::to_string(k / 4) + "]." + kSwizzle[k % 4] + ")\n";
        for (std::size_t k = 0; k < inputs.size() && k < static_cast<std::size_t>(kMaxShaderInputs); ++k)
            if (!inputs[k].empty())
                s += "#define in_" + inputName(inputs[k]) + " in" + std::to_string(k) + "\n";
        if (bodyLine != nullptr)
            *bodyLine = 1 + static_cast<int>(std::count(s.begin(), s.end(), '\n'));
        s += body;
        if (s.empty() || s.back() != '\n')
            s += '\n';
        return s;
    }

    uint32_t shaderId(const std::vector<uint8_t>& binary)
    {
        uint32_t h = 2166136261u;                    // FNV-1a
        for (const uint8_t b : binary)
            h = (h ^ b) * 16777619u;
        return h == 0 ? 1u : h;
    }

    bool shaderAnimates(const std::string& body)
    {
        const auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; };
        const std::size_t n = body.size();
        std::size_t i = 0;
        while (i < n)
        {
            const char c = body[i];
            if (c == '/' && i + 1 < n && body[i + 1] == '/')
            {
                // To the line's end; a backslash just before it joins the next line to the comment.
                for (i += 2; i < n && body[i] != '\n'; ++i)
                    if (body[i] == '\\' && i + 1 < n && body[i + 1] == '\n')
                        ++i;
                    else if (body[i] == '\\' && i + 2 < n && body[i + 1] == '\r' && body[i + 2] == '\n')
                        i += 2;
                continue;
            }
            if (c == '/' && i + 1 < n && body[i + 1] == '*')
            {
                const std::size_t end = body.find("*/", i + 2);
                i = end == std::string::npos ? n : end + 2;
                continue;
            }
            if (c == '"')
            {
                // A string (shaderText's), to its end on its line.
                for (++i; i < n && body[i] != '"' && body[i] != '\n'; ++i)
                    if (body[i] == '\\' && i + 1 < n && body[i + 1] != '\n')
                        ++i;
                if (i < n && body[i] == '"')
                    ++i;
                continue;
            }
            const auto digit = [&](std::size_t k) { return k < n && std::isdigit(static_cast<unsigned char>(body[k])) != 0; };
            if (digit(i) || (c == '.' && digit(i + 1)))
            {
                while (i < n && (word(body[i]) || body[i] == '.'))     // a number, its suffix and exponent
                    ++i;
                continue;
            }
            if (word(c))
            {
                const std::size_t from = i;
                while (i < n && word(body[i]))
                    ++i;
                const std::string_view name(body.data() + from, i - from);
                if (name == "u_time" || name == "u_viewSize")
                    return true;
                continue;
            }
            ++i;
        }
        return false;
    }

    bool shaderAnimates(const std::string& file, const std::string& body, const Sources& headers)
    {
        if (shaderAnimates(body))
            return true;
        std::string error;
        for (const std::string& name : shaderIncludes(file, body, headers, error))
            if (const SourceFile* h = findFile(headers, name); h != nullptr && shaderAnimates(h->text))
                return true;
        return false;
    }

    Sources shaderHeaders(const Sources& files)
    {
        Sources out;
        for (const SourceFile& f : files)
            if (f.name.size() > 3 && f.name.compare(f.name.size() - 3, 3, ".sh") == 0)
                out.push_back(f);
        return out;
    }

    std::string shaderText(const std::string& file, const std::string& text, std::string& error)
    {
        error.clear();
        return castOnes(walk(
            text, [](const std::string&, int, int) {},
            [&](std::string_view inner, int line, int column, bool ended) -> std::string {
                std::vector<int> codes;
                std::string why;
                if (!ended)
                    why = "a string has no closing quote on its line";
                else if (!codesOf(inner, codes, why))
                    codes.clear();
                if (!why.empty())
                {
                    if (error.empty())
                        error = prepassNote(file, line, column, why);
                    return "vec4(0.0, 0.0, 0.0, 0.0)";
                }
                return packed(codes);
            }));
    }

    std::vector<std::string> shaderIncludes(const std::string& file, const std::string& body, const Sources& headers,
                                            std::string& error)
    {
        error.clear();
        std::vector<std::string> out;
        // The files to read, in the order they are reached: the shader's, then each of the program's it includes.
        std::vector<std::pair<std::string, const std::string*>> queue{ { file, &body } };
        for (std::size_t q = 0; q < queue.size(); ++q)
        {
            const std::string from = queue[q].first;
            walk(
                *queue[q].second,
                [&](const std::string& name, int line, int column) {
                    if (name == "bgfx_shader.sh" || std::find(out.begin(), out.end(), name) != out.end())
                        return;
                    if (shaderlib::libraryFile(name) != nullptr)
                    {
                        out.push_back(name);
                        return;
                    }
                    if (const SourceFile* h = findFile(headers, name))
                    {
                        out.push_back(name);
                        queue.emplace_back(name, &h->text);
                        return;
                    }
                    if (error.empty())
                        error = prepassNote(from, line, column,
                                            "there is no file " + name + " to #include (the program's .sh files, "
                                            "funk/text.sh and funk/shapes.sh)");
                },
                [](std::string_view, int, int, bool) { return std::string(); });
        }
        return out;
    }

    ShaderFiles shaderFiles(const std::string& file, const std::string& body, const std::vector<std::string>& inputs,
                            const std::string& prelude, const Sources& headers)
    {
        ShaderFiles out;
        const std::string text = shaderText(file, body, out.error);
        if (!out.error.empty())
            return out;
        out.source = shaderSource(text, inputs, prelude, &out.bodyLine);
        for (const std::string& name : shaderIncludes(file, body, headers, out.error))
        {
            if (const shaderlib::LibraryFile* lib = shaderlib::libraryFile(name))
            {
                out.includes.emplace_back(name, castOnes(lib->text));
                continue;
            }
            const SourceFile* h = findFile(headers, name);
            if (h == nullptr)
                continue;
            std::string error;
            const std::string included = shaderText(name, h->text, error);
            if (!error.empty())
            {
                out.error = error;
                return out;
            }
            const std::string guard = guardOf(name);
            out.includes.emplace_back(name, "#ifndef " + guard + "\n#define " + guard + "\n" + included
                                                + (included.empty() || included.back() == '\n' ? "" : "\n") + "#endif\n");
        }
        return out;
    }
}
