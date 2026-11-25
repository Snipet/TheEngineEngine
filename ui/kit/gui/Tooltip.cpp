// ui/kit/gui/Tooltip.cpp: see Tooltip.h.
#include "kit/gui/Tooltip.h"

#include "kit/gui/Style.h"

#include <funkgui/core/TypeScale.h>
#include <funkgui/text/FontService.h>
#include <funkgui/text/TextFit.h>

#include <algorithm>

namespace tee::ui
{
    namespace
    {
        constexpr funkgui::TextStyle kTitle = funkgui::type::kLabel;              // 11 px spaced capitals
        constexpr funkgui::TextStyle kBody{ 12.0f, 0.4f, 0.03f, false };          // a sentence
        constexpr funkgui::TextStyle kFacts = funkgui::type::kMicro;              // 10 px
        constexpr float kPad = 9.0f;

        const funkgui::TextStyle& styleOf(int s) { return s == 0 ? kTitle : s == 1 ? kBody : kFacts; }
        float lineHeight(int s) { return s == 0 ? 16.0f : s == 1 ? 16.0f : 14.0f; }

        float widthOf(const std::string& t, int s)
        {
            return funkgui::text::width(funkgui::FontService::get().atlas(), t.c_str(), styleOf(s));
        }

        // `text` at words into lines no wider than `w` (a word wider than `w` on a line of its own).
        void wrap(const std::string& text, int style, float w, std::vector<std::string>& out)
        {
            std::string line;
            std::size_t i = 0;
            while (i < text.size())
            {
                std::size_t e = text.find(' ', i);
                if (e == std::string::npos)
                    e = text.size();
                const std::string word = text.substr(i, e - i);
                i = e + 1;
                if (word.empty())
                    continue;
                const std::string tried = line.empty() ? word : line + " " + word;
                if (!line.empty() && widthOf(tried, style) > w)
                {
                    out.push_back(line);
                    line = word;
                }
                else
                    line = tried;
            }
            if (!line.empty())
                out.push_back(line);
        }

        bool overlaps(const funkgui::Rect& a, const funkgui::Rect& b)
        {
            return a.x < b.right() && b.x < a.right() && a.y < b.bottom() && b.y < a.bottom();
        }
    }

    bool Tooltip::track(float dt, int key, float dwell)
    {
        if (key != quiet_)
            quiet_ = -1;   // the pointer left what was pressed
        else if (key >= 0)
            key = -1;
        if (key != key_)
        {
            key_ = key;
            rest_ = 0.0f;
            asked_ = false;
            hide();
            return false;
        }
        if (key < 0 || asked_)
            return false;
        rest_ += std::max(0.0f, dt);
        if (rest_ < dwell)
            return false;
        asked_ = true;
        return true;
    }

    void Tooltip::hide()
    {
        if (shown_)
            ++revision_;
        shown_ = false;
    }

    void Tooltip::show(const std::string& title, const std::string& body, const std::string& facts, const funkgui::Rect& anchor,
                       float w, float h)
    {
        lines_.clear();
        anchor_ = anchor;
        if (title.empty() && body.empty())
        {
            hide();
            return;
        }
        // The text's width: the widest line, at most kMaxWidth less the padding (and the panel's room).
        const float room = std::max(80.0f, std::min(kMaxWidth, w - 2.0f * kGap) - 2.0f * kPad);
        std::vector<std::string> parts;
        float y = kPad, widest = 0.0f;
        const auto add = [&](const std::string& t, int style) {
            parts.clear();
            wrap(t, style, room, parts);
            for (const std::string& p : parts)
            {
                lines_.push_back({ p, style, y });
                widest = std::max(widest, widthOf(p, style));
                y += lineHeight(style);
            }
        };
        if (!title.empty())
            add(title, 0);
        if (!body.empty())
        {
            if (!title.empty())
                y += 2.0f;
            add(body, 1);
        }
        if (!facts.empty())
        {
            y += 3.0f;
            add(facts, 2);
        }
        const float bw = std::min(widest + 2.0f * kPad, w - 2.0f * kGap), bh = y + kPad - 3.0f;
        // Below what it describes, else above, else right, else left; never over it, never past the edges.
        const auto clampX = [&](float x) { return std::clamp(x, kGap, std::max(kGap, w - bw - kGap)); };
        const auto clampY = [&](float yy) { return std::clamp(yy, kGap, std::max(kGap, h - bh - kGap)); };
        const funkgui::Rect tries[] = {
            { clampX(anchor.x), anchor.bottom() + kGap, bw, bh },
            { clampX(anchor.x), anchor.y - kGap - bh, bw, bh },
            { anchor.right() + kGap, clampY(anchor.y), bw, bh },
            { anchor.x - kGap - bw, clampY(anchor.y), bw, bh },
        };
        box_ = { clampX(anchor.x), clampY(anchor.bottom() + kGap), bw, bh };   // when nothing fits: inside, as near as can be
        for (const funkgui::Rect& r : tries)
            if (r.x >= kGap - 0.5f && r.y >= kGap - 0.5f && r.right() <= w - kGap + 0.5f && r.bottom() <= h - kGap + 0.5f
                && !overlaps(r, anchor))
            {
                box_ = r;
                break;
            }
        shown_ = true;
        ++revision_;
    }

    std::string Tooltip::text() const
    {
        std::string out;
        for (const Line& l : lines_)
            out += (out.empty() ? "" : "\n") + l.text;
        return out;
    }

    void Tooltip::draw(funkgui::Canvas& c, const funkgui::Theme& th) const
    {
        if (!shown_)
            return;
        const style::Palette p = style::palette(th);
        c.rrect(box_.x, box_.y, box_.w, box_.h, 5.0f, p.raised, 1.0f, p.cardEdge);
        for (const Line& l : lines_)
            c.text(l.text.c_str(), box_.x + kPad, box_.y + l.y, styleOf(l.style), l.style == 0 ? p.soft : l.style == 1 ? p.text : p.dim);
    }
}
