// ui/kit/inspect/PartCard.cpp: see PartCard.h.
#include "kit/inspect/PartCard.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace tee::ui
{
    PartCard::PartCard(FieldInspector& card) : card_(card) { card_.setPlace({}, false); }

    std::string PartCard::titleOf(const std::string& path)
    {
        std::string t = path.substr(path.rfind('.') == std::string::npos ? 0 : path.rfind('.') + 1);
        if (const std::size_t b = t.find('['); b != std::string::npos)
            t = t.substr(0, b) + " " + std::to_string(std::atoi(t.c_str() + b + 1) + 1);
        // camelCase: words
        std::string out;
        for (std::size_t i = 0; i < t.size(); ++i)
        {
            const char ch = t[i];
            if (i > 0 && std::isupper(static_cast<unsigned char>(ch)) && std::islower(static_cast<unsigned char>(t[i - 1])))
                out += ' ';
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(ch == '_' ? ' ' : ch)));
        }
        return out;
    }

    Outline PartCard::firstFields(const Outline& page, int n)
    {
        Outline out;
        int k = 0;
        for (const OutlineEntry& e : page)
        {
            if (e.kind != OutlineEntry::Kind::field && e.kind != OutlineEntry::Kind::table)
                continue;
            if (k++ >= n)
                break;
            out.push_back(e);
        }
        return out;
    }

    void PartCard::afterPress(const std::string& pathBefore, const std::string& pathNow, const std::string& hovered,
                              funkgui::Point at)
    {
        const bool chose = !pathNow.empty() && (pathNow != pathBefore || hovered == pathNow);
        if (!chose)
        {
            close();
            wanted_ = false;
            return;
        }
        wanted_ = true;
        wantedPath_ = pathNow;
        at_ = at;
    }

    void PartCard::update(const std::string& path, const Outline* page, const funkgui::Rect& stage)
    {
        if (open_ && path != path_)
            close();
        if (!wanted_ || path != wantedPath_)
            return;
        wanted_ = false;
        const Outline few = page != nullptr ? firstFields(*page, kFields) : Outline{};
        if (few.empty())
        {
            close();
            return;
        }
        const std::size_t all = fieldsOf(*page).size(), shown = fieldsOf(few).size();
        card_.setCard(titleOf(path), all > shown ? std::to_string(all - shown) + " MORE IN THE COLUMN" : std::string());
        card_.setFilter({});
        card_.setOutline(few);
        path_ = path;
        open_ = true;
        place(stage);
    }

    void PartCard::place(const funkgui::Rect& stage)
    {
        if (!open_)
            return;
        card_.setPlace({ 0.0f, 0.0f, kWidth, 2000.0f }, true);   // laid out at its width, then as tall as it holds
        const float h = std::min(34.0f + 26.0f + card_.contentHeight(), stage.h - 16.0f);
        float x = at_.x + kGap;                  // to the right of the part, else to its left
        if (x + kWidth > stage.right() - 8.0f)
            x = at_.x - kGap - kWidth;
        x = std::clamp(x, stage.x + 8.0f, std::max(stage.x + 8.0f, stage.right() - 8.0f - kWidth));
        const float y = std::clamp(at_.y - 40.0f, stage.y + 8.0f, std::max(stage.y + 8.0f, stage.bottom() - 8.0f - h));
        card_.setPlace({ x, y, kWidth, h }, true);
    }

    void PartCard::close()
    {
        if (!open_)
            return;
        open_ = false;
        path_.clear();
        card_.setPlace({}, false);
    }
}
