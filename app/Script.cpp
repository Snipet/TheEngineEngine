// app/Script.cpp: Script.h.
#include "Script.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace tee::app
{
    namespace
    {
        bool number(const std::string& text, double& out)
        {
            if (text.empty())
                return false;
            char* end = nullptr;
            out = std::strtod(text.c_str(), &end);
            return end != nullptr && *end == '\0' && out == out;
        }

        bool comparison(const std::string& op)
        {
            return op == ">" || op == "<" || op == ">=" || op == "<=" || op == "==" || op == "!=";
        }
    }

    bool parseScript(const std::string& text, Script& out, std::string& why)
    {
        out = {};
        std::istringstream in(text);
        std::string row;
        int lineNo = 0;
        double last = 0.0;
        bool ended = false;
        const auto fail = [&](const std::string& what) {
            why = "line " + std::to_string(lineNo) + ": " + what;
            return false;
        };
        while (std::getline(in, row))
        {
            ++lineNo;
            const std::size_t hash = row.find('#');
            if (hash != std::string::npos)
                row.resize(hash);
            std::istringstream words(row);
            std::vector<std::string> w;
            for (std::string word; words >> word;)
                w.push_back(word);
            if (w.empty())
                continue;
            ScriptEvent e;
            e.line = lineNo;
            if (!number(w[0], e.time) || e.time < 0.0)
                return fail("'" + w[0] + "' is no time in seconds");
            if (w.size() < 2)
                return fail("a time and nothing to do at it");
            const std::string& verb = w[1];
            const auto arg = [&](std::size_t i, double& v) { return i < w.size() && number(w[i], v); };

            if (verb == "key" && w.size() == 4 && (w[3] == "down" || w[3] == "up"))
            {
                e.kind = ScriptEvent::Kind::key;
                e.name = w[2];
                e.a = w[3] == "down" ? 1.0 : 0.0;
            }
            else if (verb == "tap" && (w.size() == 3 || w.size() == 4))
            {
                double hold = 0.1;
                if (w.size() == 4 && (!number(w[3], hold) || hold <= 0.0))
                    return fail("tap's time is seconds above 0");
                e.kind = ScriptEvent::Kind::key;
                e.name = w[2];
                e.a = 1.0;
                out.events.push_back(e);
                e.time += hold;
                e.a = 0.0;
            }
            else if (verb == "set" && w.size() == 4 && arg(3, e.a))
            {
                e.kind = ScriptEvent::Kind::set;
                e.name = w[2];
            }
            else if (verb == "pointer" && w.size() == 5 && arg(3, e.a) && arg(4, e.b)
                     && (w[2] == "move" || w[2] == "down" || w[2] == "up" || w[2] == "click"))
            {
                e.kind = ScriptEvent::Kind::pointer;
                e.name = w[2];
            }
            else if (verb == "wheel" && w.size() == 5 && arg(2, e.a) && arg(3, e.b) && arg(4, e.c))
                e.kind = ScriptEvent::Kind::wheel;
            else if (verb == "shot" && w.size() == 3)
            {
                e.kind = ScriptEvent::Kind::shot;
                e.name = w[2];
            }
            else if (verb == "expect" && w.size() == 5 && comparison(w[3]) && arg(4, e.a))
            {
                e.kind = ScriptEvent::Kind::expect;
                e.name = w[2];
                e.op = w[3];
            }
            else if (verb == "expect" && w.size() == 6 && w[2] == "param" && comparison(w[4]) && arg(5, e.a))
            {
                e.kind = ScriptEvent::Kind::expect;
                e.name = "param " + w[3];
                e.op = w[4];
            }
            else if (verb == "expect" && w.size() == 6 && w[2] == "sent" && comparison(w[4]) && arg(5, e.a))
            {
                e.kind = ScriptEvent::Kind::expect;
                e.name = "sent " + w[3];
                e.op = w[4];
            }
            else if (verb == "quality" && w.size() == 3 && (w[2] == "full" || w[2] == "economy"))
            {
                e.kind = ScriptEvent::Kind::quality;
                e.a = w[2] == "economy" ? 1.0 : 0.0;
            }
            else if (verb == "print" && w.size() == 2)
                e.kind = ScriptEvent::Kind::print;
            else if (verb == "end" && w.size() == 2)
            {
                e.kind = ScriptEvent::Kind::end;
                out.end = e.time;
                ended = true;
            }
            else
                return fail("'" + row + "' is no event (app/Script.h lists them)");
            last = std::max(last, e.time);
            out.events.push_back(e);
            if (ended)
                break;
        }
        std::stable_sort(out.events.begin(), out.events.end(),
                         [](const ScriptEvent& x, const ScriptEvent& y) { return x.time < y.time; });
        if (!ended && !out.events.empty())
            out.end = last;
        return true;
    }

    bool loadScript(const std::string& path, Script& out, std::string& why)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            why = "cannot read " + path;
            return false;
        }
        std::ostringstream text;
        text << in.rdbuf();
        if (!parseScript(text.str(), out, why))
        {
            why = path + ": " + why;
            return false;
        }
        return true;
    }

    Script defaultScript(double seconds)
    {
        // The start key for a third of a second; the throttle held from 40 % to 65 % of the time; the start key held
        // from 80 % for long enough to switch off.
        Script s;
        std::string why;
        const auto at = [seconds](double share) { return std::to_string(seconds * share); };
        const std::string text = "0.2 key return down\n0.5 key return up\n" + at(0.40) + " key w down\n" + at(0.65)
                                 + " key w up\n" + at(0.80) + " key return down\n"
                                 + std::to_string(seconds * 0.80 + 1.3) + " key return up\n";
        parseScript(text, s, why);
        s.end = seconds;
        return s;
    }

    bool compareScript(double x, const std::string& op, double y)
    {
        if (op == ">")
            return x > y;
        if (op == "<")
            return x < y;
        if (op == ">=")
            return x >= y;
        if (op == "<=")
            return x <= y;
        if (op == "==")
            return x == y;
        return x != y;
    }
}
