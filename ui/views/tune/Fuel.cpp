// ui/views/tune/Fuel.cpp: fuels.fm's rows read from its text (Fuel.h).
#include "views/tune/Fuel.h"

#include <cmath>
#include <locale>
#include <sstream>

namespace tee::ui::fuel
{
    namespace
    {
        // The numbers of `const <type> <name>[FUEL_ROWS] = {a, b, ...};` (a trailing f allowed), in order; empty when
        // the array is not there or a number does not read.
        std::vector<double> numbers(const std::string& text, const std::string& name)
        {
            const std::size_t at = text.find(" " + name + "[");
            if (at == std::string::npos)
                return {};
            const std::size_t open = text.find('{', at), close = text.find('}', at);
            if (open == std::string::npos || close == std::string::npos || close < open || text.find(';', at) < open)
                return {};
            std::vector<double> out;
            std::istringstream in(text.substr(open + 1, close - open - 1));
            in.imbue(std::locale::classic());
            while (in >> std::ws, !in.eof())
            {
                double v = 0.0;
                if (!(in >> v))
                    return {};
                out.push_back(v);
                if (in.peek() == 'f')
                    in.get();
                in >> std::ws;
                if (in.peek() == ',')
                    in.get();
                else if (!(in >> std::ws).eof())
                    return {};
            }
            return out;
        }

        // `const float <name> = <v>f;`: NaN when not there
        double scalar(const std::string& text, const std::string& name)
        {
            const std::size_t at = text.find(" " + name + " = ");
            if (at == std::string::npos)
                return std::nan("");
            std::istringstream in(text.substr(at + name.size() + 4, 32));
            in.imbue(std::locale::classic());
            double v = 0.0;
            return in >> v ? v : std::nan("");
        }
    } // namespace

    Table parse(const std::string& fuels, const std::string& knock)
    {
        Table t;
        const std::vector<double> ron = numbers(fuels, "kFuelRon"), mon = numbers(fuels, "kFuelMon");
        const std::vector<double> comp = numbers(fuels, "kFuelCompression"), untested = numbers(fuels, "kFuelUntested");
        if (ron.empty() || ron.size() != mon.size() || ron.size() != comp.size() || ron.size() != untested.size())
            return t;
        for (std::size_t i = 0; i < ron.size(); ++i)
            t.rows.push_back({ ron[i], mon[i], comp[i] != 0.0, untested[i] != 0.0 });
        const double k = scalar(fuels, "kOctaneK");
        t.k = std::isfinite(k) ? k : 0.0;
        // knockOctane: `expf(-<n>f * logf(fmaxf(octane, ...`
        const std::size_t at = knock.find("float knockOctane(");
        const std::size_t e = at == std::string::npos ? at : knock.find("expf(-", at);
        if (e != std::string::npos && knock.find("logf(fmaxf(octane", e) == knock.find(" * logf(", e) + 3)
        {
            std::istringstream in(knock.substr(e + 6, 16));
            in.imbue(std::locale::classic());
            double n = 0.0;
            if (in >> n && n > 0.0)
                t.exponent = n;
        }
        return t;
    }

    double Table::ron(int g) const
    {
        return has(g) && !rows[static_cast<std::size_t>(g)].compression && rows[static_cast<std::size_t>(g)].ron > 0.0
                   ? rows[static_cast<std::size_t>(g)].ron : std::nan("");
    }
    double Table::mon(int g) const
    {
        return has(g) && !rows[static_cast<std::size_t>(g)].compression && rows[static_cast<std::size_t>(g)].mon > 0.0
                   ? rows[static_cast<std::size_t>(g)].mon : std::nan("");
    }
    double Table::index(int g) const
    {
        const double r = ron(g), m = mon(g);
        return r - k * (r - m);
    }
    double Table::aki(int g) const
    {
        return 0.5 * (ron(g) + mon(g));
    }
    double Table::delayRatio(int g, int against) const
    {
        const double a = index(g), b = index(against);
        return exponent > 0.0 && a > 0.0 && b > 0.0 ? std::pow(a / b, exponent) : std::nan("");
    }
}
