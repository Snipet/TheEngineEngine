// ui/views/tune/Fuel.h: the fuels as the simulator has them, for TUNE's fuel control (docs/ui/tune.md). The numbers are
// read from the simulator's own source (sim/combustion/fuels.fm's rows, sim/combustion/knock.fm's octane exponent, as
// text through FrameServices::simSource), never typed in here: a row the table does not have, or a table that cannot
// be read, has no number (NaN), and the view draws a dash.
#pragma once

#include <string>
#include <vector>

namespace tee::ui::fuel
{
    struct Row
    {
        double ron = 0.0, mon = 0.0;   // the research and motor octane numbers (0: none, as a diesel's)
        bool compression = false;      // lit by compression (the diesel's burn): no octane
        bool untested = false;         // a row no test has run through an engine
    };

    struct Table
    {
        std::vector<Row> rows;         // by fuel.grade's index; empty: no table read
        double k = 0.0;                // the octane index's K (RON - K (RON - MON)), kOctaneK
        double exponent = 0.0;         // the knock delay's power of the octane index (knock.fm: 3.402); 0: not read
        // The research octanes the knock law was fitted over (fuelOctaneExtrapolated's: 80 to 100); NaN (or 0, 0):
        // not read
        double fitLo = 0.0, fitHi = 0.0;

        bool has(int grade) const { return grade >= 0 && grade < static_cast<int>(rows.size()); }
        // The octane index the knock model reads, the anti-knock index (R+M)/2 as the US pump shows it, RON and MON;
        // NaN when the table has no number for the grade (not read, out of the table, or compression-lit).
        double index(int grade) const;
        double aki(int grade) const;
        double ron(int grade) const;
        double mon(int grade) const;
        // How much longer the end gas waits before it lights on `grade` than on `against` (Douaud and Eyzat's delay,
        // knock.fm: (ON / ON0)^exponent); NaN when either has no octane or the exponent was not read.
        double delayRatio(int grade, int against) const;

        // With a stated pump number (spec.fuel.octane, AKI; 0 or less: the grade's own), as the simulator derives it
        // (fuels.fm fuelOctaneAt): RON = AKI + (RON - MON) / 2 by the grade's own sensitivity, MON that less the
        // sensitivity, the index from them. NaN where the grade has no octane.
        double ronAt(int grade, double aki) const;
        double monAt(int grade, double aki) const;
        double indexAt(int grade, double aki) const;
        // A stated number whose RON is past the knock law's fit (fuelOctaneExtrapolated); false for the grade's own,
        // a grade without octane, or a fit not read.
        bool extrapolated(int grade, double aki) const;
        // As delayRatio, `grade` at a stated number against `against`'s own.
        double delayRatioAt(int grade, double aki, int against) const;
    };

    // The numbers of a braced list of C literals ("{ 91.0f, 96.0f, 100.0F, 87 }"), in order; empty when any does not
    // read (the platform's stream parser is not used: Fuel.cpp).
    std::vector<double> numberList(const std::string& braces);

    // The table from fuels.fm's text and knock.fm's (either may be empty: then what it gives is missing).
    Table parse(const std::string& fuels, const std::string& knock);
}
