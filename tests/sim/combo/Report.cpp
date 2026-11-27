// tests/sim/combo/Report.cpp: a verdict through Catch (Combo.h, report), and the line each run prints.
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace combo
{
    namespace
    {
        int knownOf(const Verdict& v, const std::string& c, const std::string& build, const std::string& scenario)
        {
            int n = 0;
            for (const Failure& f : v.failures)
                if (known(c, build, scenario, f.check, f.event) != nullptr || shiftCutRuled(c, f))
                    ++n;
            return n;
        }
    } // namespace


    int report(const Verdict& v, const std::string& c, const std::string& build, const std::string& scenario)
    {
        // R costs three runs a process: measured only when a cost is judged (the fuzz, which asks for it) or asked
        // for (TEE_COMBO_COST=1); the fast tier's budget (01-rules.md section 3) has no room for it in every case
        const double r = std::getenv("TEE_COMBO_COST") != nullptr ? costOfR() : costOfRIfMeasured();
        // idle and revs read n/a for a scenario without that stretch (a soak, a fuzz, a swap): not a stall
        char idle[24], revs[24];
        if (v.idle > 0.0)
            std::snprintf(idle, sizeof idle, "%5.0f rpm", v.idle * 30.0 / 3.14159265);
        else
            std::snprintf(idle, sizeof idle, "%9s", "n/a");
        if (v.revMax > 0.0)
            std::snprintf(revs, sizeof revs, "%5.0f rpm", v.revMax * 30.0 / 3.14159265);
        else
            std::snprintf(revs, sizeof revs, "%9s", "n/a");
        std::printf("combo | %-40s | %-13s | %-12s | step %5.1f | dc %.4f | idle %s | revs %s | %.2f R | %4.1f %% core | "
                    "%s\n",
                    c.c_str(), build.c_str(), scenario.c_str(), v.stepMax, v.dcMax, idle, revs,
                    r > 0 ? v.coreShare / r : 0.0, 100.0 * v.coreShare,
                    v.failures.empty() ? "ok" : (v.failures.size() == static_cast<std::size_t>(knownOf(v, c, build, scenario))
                                                     ? "ok (known findings only)"
                                                     : "FAILED"));
        std::fflush(stdout);
        // a row a run for the measurements' table (docs/measurements/combo/), under out/ (never committed)
        {
            std::error_code ec;
            std::filesystem::create_directories("out/combo", ec);
            std::ofstream csv("out/combo/runs.csv", std::ios::app);
            if (csv)
                csv << c << "," << build << "," << scenario << "," << v.stepMax << "," << v.dcMax << ","
                    << v.idle * 30.0 / 3.14159265 << "," << v.revMax * 30.0 / 3.14159265 << ","
                    << (r > 0 ? v.coreShare / r : 0.0) << "," << v.coreShare << "," << v.failures.size() << "\n";
        }
        int knownCount = 0;
        for (const Failure& f : v.failures)
        {
            if (shiftCutRuled(c, f))
            {
                ++knownCount;
                WARN("ruled (the lead, 23:15): " << c << " / " << build << " / " << scenario << ": " << f.detail);
            }
            else if (const Known* k = known(c, build, scenario, f.check, f.event))
            {
                ++knownCount;
                WARN("known " << k->id << " (" << k->date << ", " << k->owner << "): " << c << " / " << build << " / "
                              << scenario << ": " << f.check << ": " << f.detail);
            }
            else
                FAIL_CHECK(c << " / " << build << " / " << scenario << ": " << f.check << ": " << f.detail);
        }
        return knownCount;
    }
} // namespace combo
