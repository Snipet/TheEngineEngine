// ui/app/LoadMeter.h: the top bar's load (the lead's ask, 2026-11-24 19:20): what the simulator takes of a core, read
// from the PEAK (HostStats::loadPeak: the mean hides the block that drops out), and a warning before a dropout, not
// after. Two widgets the frame places: LoadMeter (the figure, a bar, a word, the dropouts) and LoadAdvice (the
// sentence and its remedy, while the build does not fit). Neither switches the quality: Economy changes the sound a
// little, and that is the owner's choice.
#pragma once

#include "app/Look.h"
#include "kit/widgets/Widget.h"

#include <deque>
#include <functional>
#include <string>
#include <utility>

namespace tee::ui
{
    // What the meter and the advice read (FrameServices' functions; each may be empty).
    struct LoadSources
    {
        std::function<float()> peak;      // the largest share of a core in the last second or so (0..1+)
        std::function<float()> mean;      // the mean (FrameServices::simLoad): used when there is no peak
        std::function<int()> verdict;     // the host's own (card C): 1 fits, 0 tight, -1 over; empty: worked out here
        std::function<long()> dropouts;   // blocks the audio device missed since load (card C); empty: not shown
    };

    // The verdict from the peak held over the last few seconds: under 70 % fits (1), to 90 % tight (0), past it over
    // (-1). It goes back by itself once the held peak falls.
    class LoadWatch
    {
    public:
        static constexpr float kTight = 0.70f, kOver = 0.90f, kHold = 3.0f;   // shares of a core; seconds held
        void step(float dt, float peak);
        float held() const noexcept { return held_; }
        int verdict() const noexcept { return held_ >= kOver ? -1 : held_ >= kTight ? 0 : 1; }
        static const char* word(int verdict) { return verdict < 0 ? "over" : verdict == 0 ? "tight" : "ok"; }

    private:
        std::deque<std::pair<float, float>> seen_;   // (time, peak)
        float time_ = 0.0f, held_ = 0.0f;
    };

    // "LOAD   62 % of a core" over a bar (green under 70 % of the peak, amber from 70, red from 90, marks at both), a
    // word under it ("ok", "tight", "over": colour is never the only sign) and "dropouts: n" when n is not 0. A dash
    // without a source.
    class LoadMeter final : public Widget
    {
    public:
        LoadMeter(uint32_t id, LoadSources sources);
        float shownPeak() const noexcept { return peak_; }   // the figure shown, a share of a core (NaN: none)
        int verdict() const noexcept { return verdict_; }    // the host's, or the held peak's
        long dropouts() const noexcept { return dropouts_; }
        std::string figure() const;                          // "62 % of a core", or "-"

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        bool contains(funkgui::Point) const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return shown_ == target_; }
        const char* kind() const override { return "load"; }

    private:
        LoadSources src_;
        LoadWatch watch_;
        float peak_ = 0.0f, target_ = 0.0f, shown_ = 0.0f;
        bool has_ = false;
        int verdict_ = 1;
        long dropouts_ = 0;
    };

    // The sentence while the build does not fit: "This build is heavy for this machine at Full: Economy halves it"
    // (at Economy: "... even at Economy"), on two lines beside an amber rule. The frame puts a button by it that shows
    // the settings' QUALITY. Read by the accessibility list as one text.
    class LoadAdvice final : public Widget
    {
    public:
        LoadAdvice(uint32_t id, std::function<bool()> economy);
        // The two lines for the quality now (economy or not).
        static std::pair<std::string, std::string> lines(bool economy);

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        bool contains(funkgui::Point) const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        const char* kind() const override { return "advice"; }

    private:
        std::function<bool()> economy_;
    };
}
