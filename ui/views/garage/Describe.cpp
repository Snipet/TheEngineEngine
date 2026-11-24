// ui/views/garage/Describe.cpp: see Describe.h.
#include "views/garage/Describe.h"

#include "views/scope/Layout.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <limits>

namespace tee::ui::garage
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

        std::string lower(std::string s)
        {
            for (char& c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }
        const char* numberWord(int n)
        {
            static const char* const w[] = { "zero", "single", "twin", "three", "four", "five", "six", "seven",
                                             "eight", "nine", "ten", "eleven", "twelve" };
            return n >= 0 && n <= 12 ? w[n] : nullptr;
        }
    }

    const char* partWord(Part p) noexcept
    {
        switch (p)
        {
            case Part::whole: return "WHOLE BUILD";
            case Part::engine: return "ENGINE";
            case Part::car: return "CAR";
            case Part::tune: return "TUNE";
        }
        return "";
    }

    const FieldValue* valueOf(const BuildSummary& b, std::string_view path) noexcept
    {
        for (const FieldValue& v : b.values)
            if (v.path == path)
                return &v;
        return nullptr;
    }

    double numberOf(const BuildSummary& b, std::string_view path) noexcept
    {
        const FieldValue* v = valueOf(b, path);
        return v != nullptr && !v->v.empty() ? v->v[0] : kNaN;
    }

    std::string choiceOf(const BuildSummary& b, const FieldDoc& doc, std::string_view path)
    {
        const int f = doc.find(path);
        const double v = numberOf(b, path);
        if (f < 0 || doc.info(f).kind != FieldKind::choice || !std::isfinite(v))
            return {};
        const FieldInfo& fi = doc.info(f);
        const long i = std::lround(v - fi.min);
        return i >= 0 && i < static_cast<long>(fi.choices.size()) ? fi.choices[static_cast<std::size_t>(i)] : std::string();
    }

    std::string layoutWords(std::string_view name)
    {
        const std::string n(name);
        if (layout::isRotary(n))
            return "twin rotor";
        const int cyl = layout::cylindersOf(n);
        const char* w = numberWord(cyl);
        const bool shortInline = n.size() >= 2 && n[0] == 'I' && std::isdigit(static_cast<unsigned char>(n[1]));   // "I6"
        if ((layout::hasWord(n, "inline") || shortInline) && w != nullptr)
            return std::string("straight ") + w;
        if (layout::hasWord(n, "flat") && w != nullptr)
            return std::string("flat ") + w;
        if (layout::hasWord(n, "single"))
            return "single";
        if (layout::hasWord(n, "v-twin"))
            return "V-twin";
        if (layout::hasWord(n, "twin"))
            return "parallel twin";
        return n;   // "V8 crossplane", "V12", "W16"
    }

    double displacement(const BuildSummary& b, const FieldDoc& doc)
    {
        const double d = numberOf(b, "engine.displacement");
        if (std::isfinite(d))
            return d;
        const double bore = numberOf(b, "engine.bore"), stroke = numberOf(b, "engine.stroke");
        double cyl = numberOf(b, "engine.cylinders");
        if (!std::isfinite(cyl))
            cyl = layout::cylindersOf(choiceOf(b, doc, "engine.layout"));
        return std::isfinite(bore) && std::isfinite(stroke) && cyl > 0 ? kPi / 4.0 * bore * bore * stroke * cyl : kNaN;
    }

    std::string sentence(const BuildSummary& b, const FieldDoc& doc)
    {
        if (b.part == Part::car || b.part == Part::tune)
            return {};
        std::string out;
        const double d = displacement(b, doc);
        if (std::isfinite(d))
        {
            char buf[32];
            std::snprintf(buf, sizeof buf, "%.1f L", d * 1000.0);
            out = buf;
        }
        std::string lay = choiceOf(b, doc, "engine.layout");
        if (lay.empty())
        {
            const double n = numberOf(b, "engine.cylinders");
            if (std::isfinite(n))
                lay = std::to_string(static_cast<int>(n)) + " cylinders";
        }
        else
            lay = layoutWords(lay);
        if (!lay.empty())
            out += (out.empty() ? "" : " ") + lay;
        std::string asp = lower(choiceOf(b, doc, "boost.kind"));
        if (asp == "turbo")
            asp = "turbo";
        else if (asp == "twin turbo" || asp == "twin turbos")
            asp = "twin turbo";
        else if (asp == "quad turbos")
            asp = "quad turbo";
        else if (asp == "supercharger")
            asp = "supercharged";
        else
            asp.clear();   // natural: said by saying nothing
        if (!asp.empty())
            out += (out.empty() ? "" : ", ") + asp;
        const std::string fuel = choiceOf(b, doc, "fuel.kind");
        if (!fuel.empty() && fuel != "Gasoline" && fuel != "Petrol")
            out += (out.empty() ? "" : ", ") + (fuel == "E85" ? fuel : lower(fuel));
        return out;
    }

    std::string valueText(const FieldValue* v, const FieldDoc& doc, std::string_view path, const Units& units)
    {
        if (v == nullptr || v->v.empty())
            return "-";
        const int f = doc.find(path);
        if (f < 0)
            return formatNumber(v->v[0], -1);
        const FieldInfo& fi = doc.info(f);
        switch (fi.kind)
        {
            case FieldKind::choice:
            {
                const long i = std::lround(v->v[0] - fi.min);
                return i >= 0 && i < static_cast<long>(fi.choices.size()) ? fi.choices[static_cast<std::size_t>(i)]
                                                                         : formatNumber(v->v[0], 0);
            }
            case FieldKind::toggle: return v->v[0] >= 0.5 ? "on" : "off";
            case FieldKind::curve:
            case FieldKind::map: return "table of " + std::to_string(v->v.size()) + " numbers";
            case FieldKind::integer: return formatNumber(v->v[0], 0) + (fi.unit.empty() ? "" : " " + fi.unit);
            case FieldKind::number: break;
        }
        std::string out;
        for (std::size_t i = 0; i < v->v.size() && i < 4; ++i)
        {
            if (i > 0)
                out += ", ";
            if (fi.quantity != Quantity::none)
            {
                // Whole numbers once they are large (1150 kg, not 1150.00 kg).
                const int dec = fi.decimals >= 0 ? fi.decimals : std::fabs(units.toUser(fi.quantity, v->v[i])) >= 100.0 ? 0 : -1;
                out += units.format(fi.quantity, v->v[i], dec);
            }
            else
                out += formatNumber(v->v[i], fi.decimals) + (fi.unit.empty() ? "" : " " + fi.unit);
        }
        if (v->v.size() > 4)
            out += ", ...";
        return out;
    }

    std::string sectionWord(std::string_view section)
    {
        if (section == "ecu")
            return "TUNE";
        if (section == "car")
            return "CAR";
        if (section == "env")
            return "AIR AND ROAD";
        if (section == "sound")
            return "LISTENER";
        std::string s(section);
        for (char& c : s)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    }

    std::vector<Difference> differences(const BuildSummary& a, const BuildSummary& b, const FieldDoc& doc,
                                        const Units& units)
    {
        std::vector<Difference> out;
        for (int f = 0; f < doc.count(); ++f)
        {
            const FieldInfo& fi = doc.info(f);
            if (fi.hidden)
                continue;
            const FieldValue* va = valueOf(a, fi.path);
            const FieldValue* vb = valueOf(b, fi.path);
            if (va == nullptr && vb == nullptr)
                continue;
            bool same = va != nullptr && vb != nullptr && va->v.size() == vb->v.size();
            for (std::size_t i = 0; same && i < va->v.size(); ++i)
                same = std::fabs(va->v[i] - vb->v[i]) <= 1e-9 * std::max(1.0, std::fabs(va->v[i]));
            if (same)
                continue;
            Difference d;
            d.section = std::string(fi.section());
            d.path = fi.path;
            d.label = fi.label.empty() ? fi.path : fi.label;
            d.a = valueText(va, doc, fi.path, units);
            d.b = valueText(vb, doc, fi.path, units);
            if (d.a == d.b)   // equal as shown (a rounding apart): still a difference, said with more digits
            {
                d.a = va != nullptr && !va->v.empty() ? formatNumber(va->v[0], 4) : "-";
                d.b = vb != nullptr && !vb->v.empty() ? formatNumber(vb->v[0], 4) : "-";
            }
            out.push_back(std::move(d));
        }
        return out;
    }
}
