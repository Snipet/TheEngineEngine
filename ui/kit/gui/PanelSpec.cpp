// ui/kit/gui/PanelSpec.cpp: from TestingPlayground, Source/fminus/PanelSpec.cpp at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// Source/fminus/PanelSpec.cpp: the #pragma panel lines read (PanelSpec.h). Two passes over the lines: the displays
// first, so that a section may name a display whose line comes after it; then everything else.
#include "kit/gui/PanelSpec.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace tee::ui
{
    std::string sourceLabel(const std::string& source)
    {
        const std::size_t arrow = source.find("->");
        return source.substr(arrow == std::string::npos ? 0 : arrow + 2, kMaxDialLabel);
    }

    std::string cutText(const std::string& text, std::size_t max)
    {
        if (text.size() <= max)
            return text;
        std::size_t cut = max;
        while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0) == 0x80)   // never inside a UTF-8 sequence
            --cut;
        return text.substr(0, cut);
    }

    void readFieldAttribute(FieldLook& look, std::string_view name, const std::vector<std::string>& args)
    {
        const auto number = [&](std::size_t k, double& out) {
            if (k >= args.size() || args[k].empty())
                return false;
            // As C writes one: a float's f suffix left out (not a hex integer's F, a digit: 0x1F).
            std::string text = args[k];
            const bool hex = text.find("0x") != std::string::npos || text.find("0X") != std::string::npos;
            if ((text.back() == 'f' || text.back() == 'F') && (!hex || text.find_first_of("pP") != std::string::npos))
                text.pop_back();
            char* end = nullptr;
            const double v = std::strtod(text.c_str(), &end);
            if (end == nullptr || *end != '\0' || !std::isfinite(v))
                return false;
            out = v;
            return true;
        };
        if (name == "ui::unit" && args.size() == 1)
            look.unit = args[0];                         // whole: a display cuts it (cutText), trace shows it all
        else if (name == "ui::label" && args.size() == 1)
            look.label = args[0];
        else if (name == "ui::decimals" && args.size() == 1)
        {
            double v = 0.0;
            if (number(0, v) && v >= 0.0 && v <= 4.0 && v == std::floor(v))
                look.decimals = static_cast<int8_t>(v);
        }
        else if (name == "fminus::range" && args.size() == 2)
        {
            double lo = 0.0, hi = 0.0;
            if (number(0, lo) && number(1, hi) && hi > lo && std::fabs(lo) <= 1.0e9 && std::fabs(hi) <= 1.0e9)
            {
                look.ranged = true;
                look.lo = static_cast<float>(lo);
                look.hi = static_cast<float>(hi);
            }
        }
    }

    bool displayable(std::string_view type) noexcept
    {
        return type == "float" || type == "double" || type == "int" || type == "bool";
    }

    DisplaySource displaySource(const std::vector<PanelField>& fields, const std::string& source)
    {
        DisplaySource out;
        // The name, then its indexes: "grid[1][3]" is grid and { 1, 3 }.
        const std::size_t open = source.find('[');
        const std::string name = source.substr(0, open);
        std::vector<std::size_t> at;
        for (std::size_t k = open; k != std::string::npos && k < source.size();)
        {
            const std::size_t close = source.find(']', k);
            const std::string digits = close == std::string::npos ? std::string() : source.substr(k + 1, close - k - 1);
            if (digits.empty() || digits.size() > 9 || source[k] != '['
                || !std::all_of(digits.begin(), digits.end(), [](char c) { return c >= '0' && c <= '9'; }))
            {
                out.problem = DisplaySource::Problem::index;
                out.why = source + ": an element's index is a whole number in brackets, grid[1][3]";
                return out;
            }
            at.push_back(static_cast<std::size_t>(std::strtoul(digits.c_str(), nullptr, 10)));
            k = close + 1;
        }
        const auto f = std::find_if(fields.begin(), fields.end(), [&](const PanelField& x) { return x.name == name; });
        if (f == fields.end())
        {
            out.why = name;
            return out;
        }
        out.field = &*f;
        if (f->type.rfind("param", 0) == 0)
        {
            out.problem = DisplaySource::Problem::dial;
            return out;
        }
        // Its type's element and extents: "float[4][16]" is float, { 4, 16 }; an extent that is no number (Lang's
        // `float x[N]`) 0, unknown, which takes any index.
        const std::size_t b = f->type.find('[');
        std::string element = f->type.substr(0, b);
        while (!element.empty() && element.back() == ' ')
            element.pop_back();
        std::vector<std::size_t> extents;
        for (std::size_t k = b; k != std::string::npos; k = f->type.find('[', k + 1))
        {
            char* end = nullptr;
            const unsigned long n = std::strtoul(f->type.c_str() + k + 1, &end, 10);
            extents.push_back(end != nullptr && *end == ']' ? static_cast<std::size_t>(n) : 0);
        }
        const auto shape = [&] {
            std::string example = name;
            for (std::size_t k = 0; k < extents.size(); ++k)
                example += "[" + std::to_string(k == 0 ? 1 : 3) + "]";
            return name + " is a " + f->type + ": an element shows, an index a dimension (" + example + ")";
        };
        if (!displayable(element) || at.size() != extents.size())
        {
            out.problem = DisplaySource::Problem::type;
            out.why = !displayable(element) ? name + " is a " + f->type
                    : extents.empty()       ? name + " is a " + f->type + ", no array"
                                            : shape();
            return out;
        }
        std::size_t index = 0;
        for (std::size_t k = 0; k < at.size(); ++k)
        {
            if (extents[k] != 0 && at[k] >= extents[k])
            {
                out.problem = DisplaySource::Problem::index;
                out.why = source + " is outside " + name + ", a " + f->type;
                return out;
            }
            index = index * (extents[k] != 0 ? extents[k] : 1) + at[k];
        }
        out.problem = DisplaySource::Problem::none;
        out.type = element;
        out.element = index;
        return out;
    }

    std::vector<PanelField> displayFields(const std::vector<PanelField>& fields, const std::vector<PanelField>& shared)
    {
        const auto squeezed = [](std::string t) {
            t.erase(std::remove(t.begin(), t.end(), ' '), t.end());
            return t;
        };
        std::vector<PanelField> out = fields;
        const auto via = std::find_if(fields.begin(), fields.end(), [&](const PanelField& f) { return squeezed(f.type) == "Shared*"; });
        if (via == fields.end())
            return out;
        for (const PanelField& f : shared)
            if (f.type.rfind("param", 0) != 0)          // its params are no dials: left at their defaults
                out.push_back({ via->name + "->" + f.name, f.type, f.look });
        return out;
    }
}
