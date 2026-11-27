// ui/app/BakeStatus.h: the calibration bake's state as the interface reads it, and its words (U1, 27 Nov; card C's
// docs/plan/bake-across-edits.md, options 1 and 2, the owner's yes): after an edit the engine plays on the last good
// tables, marked stale, while the new ones bake, and a bake waits for the edits to rest. The host's fields map into this
// in ServiceBake (app/host/HostSources.h); a state as one string (FrameServices::sparkMap, as before) parses into it too,
// so every place that says the bake says it in the same words: the frame's status line and TUNE.
#pragma once

#include <cmath>
#include <string>

namespace tee::ui
{
    struct BakeStatus
    {
        enum class Phase
        {
            none,       // nothing to say: no table, or not served
            resting,    // waiting for the edits to rest before it bakes
            baking,
            checking,   // a cached bake checked on this program (seconds, not minutes)
            baked,
            checked,    // "calibration checked": the cached bake taken
            failed,
        };
        Phase phase = Phase::none;
        std::string stage;                    // "spark map", "cam", "spark map again" ("" none chosen yet)
        int done = 0, total = 0;              // the stage's count (total 0: no count)
        bool staleSpark = false, staleCam = false;   // the engine plays on that table from before the edit
        std::string restartedBy;              // the field whose edit restarted the bake, as the interface names it ("" none)
        std::string kept;                     // the stages kept, unchanged by the edit ("spark map"; "spark map and cam")
        bool setAside = false;                // a stale table not fit (another build; a table's axes moved): set aside
        std::string error;

        bool stale() const noexcept { return staleSpark || staleCam; }
        bool working() const noexcept { return phase == Phase::resting || phase == Phase::baking || phase == Phase::checking; }
    };

    // ServiceBake's state as one string ("waiting to bake", "baking cam 23 of 42", "checking spark map 3 of 9", "baked",
    // "calibration checked", "bake failed", "no table"), as FrameServices::sparkMap gives it, into a BakeStatus.
    BakeStatus bakeStatusOf(const std::string& state);

    // The status line's words while the bake works or failed, in fields "   " apart (the line drops whole fields from its
    // end when short of room): what the bake does, then why it is where it is. "" when there is nothing to say.
    //   "Bakes once your edits rest" (no number: the rest is a few seconds' debounce, not counted down under the cursor)
    //   "Baking the spark map 12 of 80"
    //   "Baking the spark map 0 of 80   restarted: compression changed"
    //   "Baking the cam 3 of 56   spark map kept, unchanged by the edit"
    std::string bakeLine(const BakeStatus& b);
    // The second line, under it, while the engine plays on tables from before the edit (option 1): "Old tables in use
    // until baked", "Old spark map in use until baked", "Old cam in use until baked"; "" when nothing stale or set aside.
    std::string bakeStaleLine(const BakeStatus& b);
    // The bake alone, lower case, for a sentence that carries it ("baking the spark map 12 of 80"); "" when not working.
    std::string bakeDoing(const BakeStatus& b);
    // The stale words, lower case ("old tables in use until baked"); "" when nothing stale.
    std::string bakeStaleWords(const BakeStatus& b);
    // The state in a few words, lower case, for a label that carries it (TUNE's "Knock limit: ..."): while working,
    // bakeDoing's; else "baked", "calibration checked", "bake failed"; "" for none.
    std::string bakeState(const BakeStatus& b);
}
