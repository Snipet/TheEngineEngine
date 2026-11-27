#include "app/BakeStatus.h"

#include <cstdlib>

namespace tee::ui
{
    namespace
    {
        std::string countOf(const BakeStatus& b)
        {
            return b.total > 0 ? " " + std::to_string(b.done) + " of " + std::to_string(b.total) : std::string();
        }

        std::string capital(std::string s)
        {
            if (!s.empty() && s[0] >= 'a' && s[0] <= 'z')
                s[0] = static_cast<char>(s[0] - 'a' + 'A');
            return s;
        }
    }

    BakeStatus bakeStatusOf(const std::string& state)
    {
        BakeStatus b;
        std::string s = state;
        // "... 23 of 42": the count off the end
        if (const std::size_t of = s.rfind(" of "); of != std::string::npos)
            if (const std::size_t sp = s.rfind(' ', of - 1); sp != std::string::npos)
            {
                b.done = std::atoi(s.c_str() + sp + 1);
                b.total = std::atoi(s.c_str() + of + 4);
                s = s.substr(0, sp);
            }
        if (s == "waiting to bake")
            b.phase = BakeStatus::Phase::resting;
        else if (s == "baked")
            b.phase = BakeStatus::Phase::baked;
        else if (s == "calibration checked")
            b.phase = BakeStatus::Phase::checked;
        else if (s == "bake failed")
            b.phase = BakeStatus::Phase::failed;
        else if (s.rfind("checking", 0) == 0)
        {
            b.phase = BakeStatus::Phase::checking;
            if (s.rfind("checking ", 0) == 0 && s != "checking calibration")
                b.stage = s.substr(9);
        }
        else if (s.rfind("baking", 0) == 0)
        {
            b.phase = BakeStatus::Phase::baking;
            if (s.size() > 7)
                b.stage = s.substr(7);
        }
        return b;
    }

    std::string bakeDoing(const BakeStatus& b)
    {
        switch (b.phase)
        {
            case BakeStatus::Phase::resting:   // no number: the rest is a debounce, not counted down under the cursor
                return "bakes once your edits rest";
            case BakeStatus::Phase::checking:
                return "checking the calibration" + countOf(b);
            case BakeStatus::Phase::baking:
                return (b.stage.empty() ? std::string("baking the calibration") : "baking the " + b.stage) + countOf(b);
            default:
                return {};
        }
    }

    std::string bakeState(const BakeStatus& b)
    {
        switch (b.phase)
        {
            case BakeStatus::Phase::baked: return "baked";
            case BakeStatus::Phase::checked: return "calibration checked";
            case BakeStatus::Phase::failed: return "bake failed";
            default: return bakeDoing(b);
        }
    }

    std::string bakeStaleWords(const BakeStatus& b)
    {
        if (!b.working() || !b.stale() || b.setAside)
            return {};
        const char* what = b.staleSpark && b.staleCam ? "old tables" : b.staleSpark ? "old spark map" : "old cam";
        return std::string(what) + " in use until baked";
    }

    std::string bakeLine(const BakeStatus& b)
    {
        if (b.phase == BakeStatus::Phase::failed)
            return "Calibration bake failed";
        if (!b.working())
            return {};
        // the doing first
        std::string line = capital(bakeDoing(b));
        // then why it is where it is, a field of its own the line drops first when short of room
        if (!b.restartedBy.empty())
            line += "   restarted: " + b.restartedBy + " changed";
        else if (!b.kept.empty())
            line += "   " + b.kept + " kept, unchanged by the edit";
        return line;
    }

    std::string bakeStaleLine(const BakeStatus& b) { return capital(bakeStaleWords(b)); }
}
