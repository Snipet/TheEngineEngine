// ui/app/TryFirst.cpp: see TryFirst.h.
#include "app/TryFirst.h"

#include <funkgui/text/FontService.h>
#include <funkgui/text/TextFit.h>

#include <algorithm>

namespace tee::ui
{
    namespace
    {
        float widthOf(const std::string& s, const funkgui::TextStyle& st)
        {
            return funkgui::text::width(funkgui::FontService::get().atlas(), s.c_str(), st);
        }

        std::string trim(const std::string& s)
        {
            const std::size_t a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos)
                return {};
            const std::size_t b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        }
    }

    TryFirstCard::TryFirstCard(uint32_t id) : Widget(id)
    {
        title_ = "Try first";
    }

    void TryFirstCard::setText(std::string name, const std::string& notes)
    {
        name_ = std::move(name);
        // The opening paragraph, its line breaks as spaces.
        std::string first = notes.substr(0, notes.find("\n\n"));
        std::replace(first.begin(), first.end(), '\n', ' ');
        opening_ = trim(first);
        lines_ = wrap(opening_, bounds_.w - 2.0f * kPad);
    }

    std::vector<std::string> TryFirstCard::wrap(const std::string& text, float width)
    {
        std::vector<std::string> lines;
        if (width <= 20.0f)
            return lines;
        std::string line;
        std::size_t i = 0;
        bool cut = false;
        while (i < text.size())
        {
            const std::size_t e = std::min(text.find(' ', i), text.size());
            const std::string word = text.substr(i, e - i);
            i = e + 1;
            if (word.empty())
                continue;
            const std::string tryLine = line.empty() ? word : line + " " + word;
            if (line.empty() || widthOf(tryLine, look::kBody) <= width)
            {
                line = tryLine;
                continue;
            }
            lines.push_back(line);
            line = word;
            if (static_cast<int>(lines.size()) == kMaxLines)
            {
                cut = true;
                break;
            }
        }
        if (!cut && !line.empty())
            lines.push_back(line);
        if (cut)
        {
            // The last line with an ellipsis that fits.
            std::string& last = lines.back();
            while (!last.empty() && widthOf(last + "...", look::kBody) > width)
                last.erase(last.find_last_of(' ') == std::string::npos ? last.size() - 1 : last.find_last_of(' '));
            last += "...";
        }
        return lines;
    }

    float TryFirstCard::heightFor(float width) const
    {
        const auto n = static_cast<float>(wrap(opening_, width - 2.0f * kPad).size());
        return kPad + kHeadH + n * kLineH + kPad - 4.0f;
    }

    void TryFirstCard::setPlace(const funkgui::Rect& r, bool shown)
    {
        if (r.w != bounds_.w)
            lines_ = wrap(opening_, r.w - 2.0f * kPad);
        Widget::setPlace(r, shown);
    }

    uint64_t TryFirstCard::drawKey() const { return DrawKey{}.mix(name_).mix(opening_).mix(bounds_.w).h; }

    void TryFirstCard::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const look::Palette p = look::Palette::of(th);
        const funkgui::Rect& b = bounds_;
        c.rrect(b.x, b.y, b.w, b.h, look::kRadius, p.raised, 1.0f, p.edge);
        c.rrect(b.x, b.y + 8.0f, 3.0f, b.h - 16.0f, 1.5f, p.selected);   // its mark: a rule at the left
        const float x = b.x + kPad;
        c.text("TRY FIRST", x, b.y + kPad, look::kCaps, p.selected);
        c.text(name_.c_str(), x, b.y + kPad + 16.0f, look::kTitle, p.value);
        float y = b.y + kPad + kHeadH;
        for (const std::string& l : lines_)
        {
            c.text(l.c_str(), x, y, look::kBody, p.text);
            y += kLineH;
        }
    }

    void TryFirstCard::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it;
        it.id = id_;
        it.role = funkgui::A11yRole::staticText;
        it.bounds = bounds_;
        it.title = title_;
        it.value = name_ + ". " + opening_;
        it.readOnly = true;
        out.push_back(std::move(it));
    }
}
