// ui/app/LoadMeter.cpp: see LoadMeter.h.
#include "app/LoadMeter.h"

#include "kit/gui/Items.h"

#include <funkgui/core/Ease.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tee::ui
{
    namespace
    {
        using look::Palette;

        funkgui::A11yItem textItem(uint32_t id, const funkgui::Rect& r, std::string title, std::string value)
        {
            funkgui::A11yItem it;
            it.id = id;
            it.role = funkgui::A11yRole::staticText;
            it.bounds = r;
            it.title = std::move(title);
            it.value = std::move(value);
            it.readOnly = true;
            return it;
        }

        funkgui::Col inkOf(const Palette& p, int verdict) { return verdict < 0 ? p.hotter : verdict == 0 ? p.warning : p.ok; }
    }

    // ---- LoadWatch --------------------------------------------------------------------------------------------------
    void LoadWatch::step(float dt, float peak)
    {
        time_ += std::max(0.0f, dt);
        if (std::isfinite(peak))
            seen_.emplace_back(time_, std::max(0.0f, peak));
        while (!seen_.empty() && seen_.front().first < time_ - kHold)
            seen_.pop_front();
        held_ = 0.0f;
        for (const auto& s : seen_)
            held_ = std::max(held_, s.second);
    }

    // ---- LoadMeter --------------------------------------------------------------------------------------------------
    LoadMeter::LoadMeter(uint32_t id, LoadSources sources) : Widget(id), src_(std::move(sources))
    {
        title_ = "Load";
    }

    void LoadMeter::step(float dt)
    {
        has_ = static_cast<bool>(src_.peak) || static_cast<bool>(src_.mean);
        peak_ = src_.peak ? src_.peak() : src_.mean ? src_.mean() : 0.0f;
        if (!std::isfinite(peak_))
            peak_ = 0.0f;
        watch_.step(dt, peak_);
        verdict_ = src_.verdict ? std::clamp(src_.verdict(), -1, 1) : has_ ? watch_.verdict() : 1;
        dropouts_ = src_.dropouts ? std::max(0L, src_.dropouts()) : 0;
        target_ = std::clamp(peak_, 0.0f, 1.0f);
        shown_ = funkgui::ease::toward(shown_, target_, dt, 0.04f, 1e-3f);
    }

    std::string LoadMeter::figure() const
    {
        if (!has_)
            return "-";
        char b[32];
        std::snprintf(b, sizeof b, "%d %% of a core", static_cast<int>(std::lround(peak_ * 100.0f)));
        return b;
    }

    uint64_t LoadMeter::drawKey() const
    {
        return DrawKey{}.mix(figure()).mix(verdict_).mix(static_cast<int>(dropouts_)).mix(shown_).h;
    }

    void LoadMeter::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Palette p = Palette::of(th);
        const funkgui::Rect& b = bounds_;
        c.text("LOAD", b.x, b.y + 1.0f, look::kCaps, p.label);
        c.text(figure().c_str(), b.right(), b.y, look::kBody, has_ ? p.value : p.dim, funkgui::Align::right);
        // The bar, its marks at 70 and 90 % of a core.
        const funkgui::Rect t{ b.x, b.y + 17.0f, b.w, 7.0f };
        c.rrect(t.x, t.y, t.w, t.h, 2.5f, p.raised, 1.0f, p.edge);
        const float w = (t.w - 2.0f) * shown_;
        if (has_ && w > 0.5f)
            c.rrect(t.x + 1.0f, t.y + 1.0f, w, t.h - 2.0f, 2.0f, inkOf(p, verdict_));
        for (const float m : { LoadWatch::kTight, LoadWatch::kOver })
        {
            const float x = c.snapX(t.x + t.w * m);
            c.segment(x, t.y - 2.0f, x, t.bottom() + 2.0f, 1.0f, p.label);
        }
        // The word, and the dropouts when there were any.
        if (has_)
        {
            std::string word = LoadWatch::word(verdict_);
            for (char& ch : word)
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            c.text(word.c_str(), b.x, b.y + 29.0f, look::kCaps, verdict_ > 0 ? p.label : inkOf(p, verdict_));
        }
        if (dropouts_ > 0)
        {
            const std::string d = "DROPOUTS: " + std::to_string(dropouts_);
            c.text(d.c_str(), b.right(), b.y + 29.0f, look::kCaps, p.hotter, funkgui::Align::right);
        }
    }

    void LoadMeter::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        std::string v = figure();
        if (has_)
            v += std::string(", ") + LoadWatch::word(verdict_);
        if (dropouts_ > 0)
            v += ", dropouts: " + std::to_string(dropouts_);
        out.push_back(textItem(id_, bounds_, title_, v));
    }

    // ---- LoadAdvice -------------------------------------------------------------------------------------------------
    LoadAdvice::LoadAdvice(uint32_t id, std::function<bool()> economy) : Widget(id), economy_(std::move(economy))
    {
        title_ = "Load warning";
    }

    std::pair<std::string, std::string> LoadAdvice::lines(bool economy)
    {
        if (economy)
            return { "This build is heavy for this machine", "even at Economy" };
        return { "This build is heavy for this machine at Full:", "Economy halves it" };
    }

    uint64_t LoadAdvice::drawKey() const { return DrawKey{}.mix(economy_ && economy_() ? 1 : 0).h; }

    void LoadAdvice::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Palette p = Palette::of(th);
        const auto [a, b] = lines(economy_ && economy_());
        const funkgui::Rect& r = bounds_;
        c.rrect(r.x, r.y + 1.0f, 3.0f, r.h - 2.0f, 1.5f, p.warning);
        const float x = r.x + 10.0f, w = r.w - 10.0f;
        c.text(fitText(c, a, look::kBody, w).c_str(), x, r.y + 1.0f, look::kBody, p.text);
        c.text(fitText(c, b, look::kBody, w).c_str(), x, r.y + 17.0f, look::kBody, p.warning);
    }

    void LoadAdvice::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        const auto [a, b] = lines(economy_ && economy_());
        out.push_back(textItem(id_, bounds_, title_, a + " " + b));
    }
}
