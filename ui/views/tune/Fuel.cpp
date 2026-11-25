// ui/views/tune/Fuel.cpp: fuels.fm's rows read from its text (Fuel.h).
#include "views/tune/Fuel.h"

#include <cctype>
#include <cmath>
#include <cstdlib>

namespace tee::ui::fuel
{
    namespace
    {
        // One number of the simulator's text at `at`: a C literal ("91.0f", "1100.0e3f", "-3.402f", "87"), its float
        // suffix dropped. The characters a decimal literal may hold are taken first and converted with strtod (the
        // program never changes the C locale), never by a stream's >> double: libc++'s gathers the suffix 'f' into the
        // number and fails. False, `at` unmoved, when no number starts there.
        bool number(const std::string& text, std::size_t& at, double& v)
        {
            std::size_t i = at;
            if (i < text.size() && (text[i] == '-' || text[i] == '+'))
                ++i;
            const std::size_t digits = i;
            while (i < text.size() && (std::isdigit(static_cast<unsigned char>(text[i])) != 0 || text[i] == '.'))
                ++i;
            if (i == digits)
                return false;
            if (i < text.size() && (text[i] == 'e' || text[i] == 'E'))
            {
                std::size_t j = i + 1;
                if (j < text.size() && (text[j] == '-' || text[j] == '+'))
                    ++j;
                const std::size_t ex = j;
                while (j < text.size() && std::isdigit(static_cast<unsigned char>(text[j])) != 0)
                    ++j;
                if (j > ex)
                    i = j;
            }
            const std::string token = text.substr(at, i - at);
            char* end = nullptr;
            const double r = std::strtod(token.c_str(), &end);
            if (end != token.c_str() + token.size() || !std::isfinite(r))
                return false;
            if (i < text.size() && (text[i] == 'f' || text[i] == 'F'))
                ++i;
            v = r;
            at = i;
            return true;
        }
        void spaces(const std::string& text, std::size_t& at)
        {
            while (at < text.size() && std::isspace(static_cast<unsigned char>(text[at])) != 0)
                ++at;
        }
    } // namespace

    std::vector<double> numberList(const std::string& braces)
    {
        std::size_t at = 0;
        spaces(braces, at);
        if (at >= braces.size() || braces[at] != '{')
            return {};
        ++at;
        std::vector<double> out;
        for (;;)
        {
            spaces(braces, at);
            if (at < braces.size() && braces[at] == '}' && out.empty())
                return out;
            double v = 0.0;
            if (!number(braces, at, v))
                return {};
            out.push_back(v);
            spaces(braces, at);
            if (at < braces.size() && braces[at] == ',')
            {
                ++at;
                continue;
            }
            if (at < braces.size() && braces[at] == '}')
                return out;
            return {};
        }
    }

    namespace
    {
        // The numbers of `const <type> <name>[FUEL_ROWS] = {a, b, ...};` in order; empty when the array is not there
        // or a number does not read.
        std::vector<double> numbers(const std::string& text, const std::string& name)
        {
            const std::size_t at = text.find(" " + name + "[");
            if (at == std::string::npos)
                return {};
            const std::size_t open = text.find('{', at), close = text.find('}', at);
            if (open == std::string::npos || close == std::string::npos || close < open || text.find(';', at) < open)
                return {};
            return numberList(text.substr(open, close - open + 1));
        }

        // `const float <name> = <v>f;`: NaN when not there
        double scalar(const std::string& text, const std::string& name)
        {
            const std::size_t at = text.find(" " + name + " = ");
            if (at == std::string::npos)
                return std::nan("");
            std::size_t i = at + name.size() + 4;
            double v = 0.0;
            return number(text, i, v) ? v : std::nan("");
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
            std::size_t i = e + 6;
            double n = 0.0;
            if (number(knock, i, n) && n > 0.0)
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
