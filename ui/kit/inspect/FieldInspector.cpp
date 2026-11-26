// ui/kit/inspect/FieldInspector.cpp: see FieldInspector.h.
#include "kit/inspect/FieldInspector.h"

#include "kit/bind/Binding.h"
#include "kit/widgets/Gauge.h"

#include <funkgui/panel/HostServices.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace tee::ui
{
    namespace
    {
        using funkgui::Align;
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;

        constexpr float kPad = 10.0f;
        constexpr funkgui::TextStyle kLabelText{ 12.0f, 0.3f, 0.03f, false };   // a field's name
        constexpr funkgui::TextStyle kValueText{ 12.0f, 0.0f, 0.0f, false };    // its value
        constexpr funkgui::TextStyle kHeadText{ 10.0f, 2.0f, 0.08f, false };    // a heading
        constexpr float kWrap = 0.62f;                                           // the face's advance, of its px

        std::string lower(std::string s)
        {
            for (char& ch : s)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            return s;
        }

        void textAt(Canvas& c, const std::string& s, float x, float cy, const funkgui::TextStyle& st, Col col, Align a = Align::left)
        {
            c.text(s.c_str(), x, c.capCentreTop(cy, st), st, col, a);
        }

        // `s` broken into lines of at most `chars` characters at spaces.
        std::vector<std::string> wrap(const std::string& s, std::size_t chars)
        {
            std::vector<std::string> out;
            std::string line;
            std::size_t i = 0;
            chars = std::max<std::size_t>(chars, 8);
            while (i < s.size())
            {
                const std::size_t sp = std::min(s.find(' ', i), s.size());
                const std::string word = s.substr(i, sp - i);
                if (!line.empty() && line.size() + 1 + word.size() > chars)
                {
                    out.push_back(line);
                    line.clear();
                }
                line += (line.empty() ? "" : " ") + word;
                i = sp + 1;
            }
            if (!line.empty())
                out.push_back(line);
            return out;
        }

        bool numberKind(const FieldInfo& f) noexcept { return f.kind == FieldKind::number || f.kind == FieldKind::integer; }
        constexpr int kIds = 62;
    }

    FieldInspector::FieldInspector(uint32_t id, FieldDoc& doc, const Units& units, Style style)
        : Widget(id), doc_(doc), units_(units), style_(style)
    {
    }

    // ---- what it shows -----------------------------------------------------------------------------------------------
    void FieldInspector::setOutline(Outline outline)
    {
        outline_ = std::move(outline);
        rebuild();
    }

    void FieldInspector::setSearchable(std::vector<int> fields)
    {
        searchable_ = std::move(fields);
        rebuild();
    }

    void FieldInspector::setCard(std::string title, std::string more)
    {
        card_ = true;
        cardTitle_ = std::move(title);
        cardMore_ = std::move(more);
        layout();
    }

    void FieldInspector::setFilter(std::string text)
    {
        filter_.buffer = std::move(text);
        filter_.caret = static_cast<int>(filter_.buffer.size());
        filter_.allSelected = false;
        scroll_ = shownScroll_ = 0.0f;
        rebuild();
    }

    void FieldInspector::setChangedOnly(bool on)
    {
        changedOnly_ = on;
        scroll_ = shownScroll_ = 0.0f;
        rebuild();
    }

    void FieldInspector::fold(const std::string& heading, bool folded)
    {
        std::erase(folded_, heading);
        if (folded)
            folded_.push_back(heading);
        layout();
    }

    bool FieldInspector::matches(int field) const
    {
        const FieldInfo& f = doc_.info(field);
        const std::string hay = lower(f.label + " " + f.path + " " + f.group + " " + f.arrayLabel);
        std::string words = lower(filter_.buffer);
        std::size_t i = 0;
        while (i < words.size())
        {
            const std::size_t sp = std::min(words.find(' ', i), words.size());
            if (sp > i && hay.find(words.substr(i, sp - i)) == std::string::npos)
                return false;
            i = sp + 1;
        }
        return true;
    }

    bool FieldInspector::changed(int field) const
    {
        const FieldInfo& f = doc_.info(field);
        if (f.table())
            return false;
        const double v = doc_.get(field), d = clampTo(f, f.def);
        return std::fabs(v - d) > 1e-9 * (1.0 + std::fabs(d));
    }

    void FieldInspector::rebuild()
    {
        if (!filter_.buffer.empty())
        {
            std::vector<int> found;
            for (const int f : searchable_.empty() ? fieldsOf(outline_) : searchable_)
                if (!doc_.info(f).hidden && matches(f))
                    found.push_back(f);
            filtered_ = outlineOf(doc_, found);
        }
        else
            filtered_ = outline_;
        if (changedOnly_)
        {
            Outline kept;
            for (const OutlineEntry& e : filtered_)
            {
                if (e.kind == OutlineEntry::Kind::field && !changed(e.field))
                    continue;
                if (e.kind == OutlineEntry::Kind::table
                    && std::none_of(e.fields.begin(), e.fields.end(), [&](int f) { return changed(f); }))
                    continue;
                if (e.kind == OutlineEntry::Kind::note)
                    continue;
                kept.push_back(e);
            }
            Outline tidy;
            for (std::size_t i = 0; i < kept.size(); ++i)
                if (kept[i].kind != OutlineEntry::Kind::heading
                    || (i + 1 < kept.size() && kept[i + 1].kind != OutlineEntry::Kind::heading))
                    tidy.push_back(kept[i]);
            filtered_ = std::move(tidy);
        }
        docSeen_ = doc_.revision();
        layout();
    }

    // ---- layout ------------------------------------------------------------------------------------------------------
    float FieldInspector::headerH() const noexcept
    {
        if (card_)
            return 34.0f;
        return 56.0f;
    }

    Rect FieldInspector::contentRect() const noexcept
    {
        const float top = headerH(), bottom = card_ ? 26.0f : 0.0f;
        return { bounds_.x, bounds_.y + top, bounds_.w, std::max(0.0f, bounds_.h - top - bottom) };
    }

    Rect FieldInspector::filterRect() const noexcept
    {
        if (card_)
            return {};
        return { bounds_.x + kPad, bounds_.y + 8.0f, bounds_.w - 2.0f * kPad - 96.0f, 26.0f };
    }

    Rect FieldInspector::changedRect() const noexcept
    {
        if (card_)
            return {};
        return { bounds_.right() - kPad - 88.0f, bounds_.y + 8.0f, 88.0f, 26.0f };
    }

    void FieldInspector::setPlace(const Rect& r, bool shown)
    {
        const bool moved = r.w != bounds_.w || r.h != bounds_.h || r.x != bounds_.x || r.y != bounds_.y;
        Widget::setPlace(r, shown);
        if (moved)
            layout();
    }

    void FieldInspector::layout()
    {
        rows_.clear();
        const float x0 = bounds_.x + (card_ ? 4.0f : 0.0f), w = bounds_.w - (card_ ? 8.0f : 0.0f);
        const bool compact = style_ == Style::list;
        float y = 4.0f;
        bool folded = false;
        for (std::size_t i = 0; i < filtered_.size(); ++i)
        {
            const OutlineEntry& e = filtered_[i];
            Row r;
            r.entry = static_cast<int>(i);
            switch (e.kind)
            {
                case OutlineEntry::Kind::heading:
                {
                    folded = std::find(folded_.begin(), folded_.end(), e.text) != folded_.end() && filter_.buffer.empty();
                    r.kind = Row::Kind::heading;
                    r.folded = folded;
                    const float h = compact ? 26.0f : 30.0f;
                    r.rect = { x0, y, w, h };
                    y += h;
                    rows_.push_back(r);
                    continue;
                }
                case OutlineEntry::Kind::note:
                {
                    if (folded)
                        continue;
                    r.kind = Row::Kind::note;
                    const std::size_t chars = static_cast<std::size_t>((w - 2.0f * kPad) / (10.0f * kWrap));
                    const float h = 8.0f + 13.0f * static_cast<float>(wrap(e.text, chars).size());
                    r.rect = { x0, y, w, h };
                    y += h;
                    rows_.push_back(r);
                    continue;
                }
                case OutlineEntry::Kind::table:
                {
                    if (folded)
                        continue;
                    r.kind = Row::Kind::table;
                    const float h = compact ? 30.0f : 40.0f;
                    r.rect = { x0, y, w, h };
                    r.value = { x0 + w - kPad - 70.0f, y + (h - 24.0f) * 0.5f, 70.0f, 24.0f };
                    y += h;
                    rows_.push_back(r);
                    continue;
                }
                case OutlineEntry::Kind::field: break;
            }
            if (folded)
                continue;
            r.kind = Row::Kind::field;
            r.field = e.field;
            const FieldInfo& f = doc_.info(e.field);
            const float h = compact ? 30.0f : (numberKind(f) ? 42.0f : 36.0f);
            r.rect = { x0, y, w, h };
            const float bw = compact ? 112.0f : 122.0f, bh = compact ? 22.0f : 24.0f;
            if (f.kind == FieldKind::toggle)
                r.value = { x0 + w - kPad - 40.0f, y + (h - 20.0f) * 0.5f, 40.0f, 20.0f };
            else
                r.value = { x0 + w - kPad - bw, y + (compact ? (h - bh) * 0.5f : 5.0f), bw, bh };
            y += h;
            rows_.push_back(r);
        }
        contentH_ = y + 8.0f;
        scrollTo(scroll_);
        if (focus_ >= static_cast<int>(rows_.size()))
            focus_ = -1;
    }

    Rect FieldInspector::onScreen(const Rect& r) const noexcept
    {
        const Rect c = contentRect();
        return { r.x, c.y + r.y - shownScroll_, r.w, r.h };
    }

    void FieldInspector::scrollTo(float s)
    {
        const float most = std::max(0.0f, contentH_ - contentRect().h);
        scroll_ = std::clamp(s, 0.0f, most);
    }

    void FieldInspector::keepInView(int row)
    {
        if (row < 0 || row >= static_cast<int>(rows_.size()))
            return;
        const Rect& r = rows_[static_cast<std::size_t>(row)].rect;
        const float h = contentRect().h, sticky = 30.0f;
        if (r.y - sticky < scroll_)
            scrollTo(r.y - sticky);
        else if (r.bottom() > scroll_ + h)
            scrollTo(r.bottom() - h);
    }

    int FieldInspector::rowAt(funkgui::Point p) const
    {
        const Rect c = contentRect();
        if (!c.contains(p))
            return -1;
        for (std::size_t i = 0; i < rows_.size(); ++i)
            if (onScreen(rows_[i].rect).contains(p))
                return static_cast<int>(i);
        return -1;
    }

    bool FieldInspector::fieldRect(int field, Rect& row, Rect& value) const
    {
        const Rect c = contentRect();
        for (const Row& r : rows_)
            if (r.kind == Row::Kind::field && r.field == field)
            {
                row = onScreen(r.rect);
                value = onScreen(r.value);
                return row.bottom() > c.y && row.y < c.bottom();
            }
        return false;
    }

    int FieldInspector::focusedField() const noexcept
    {
        return focus_ >= 0 && focus_ < static_cast<int>(rows_.size()) ? rows_[static_cast<std::size_t>(focus_)].field : -1;
    }

    int FieldInspector::shownFields() const noexcept
    {
        int n = 0;
        for (const OutlineEntry& e : filtered_)
            n += e.kind == OutlineEntry::Kind::field ? 1 : static_cast<int>(e.fields.size());
        return n;
    }

    void FieldInspector::focusRow(int row)
    {
        focus_ = row;
        keepInView(row);
    }

    // ---- values ------------------------------------------------------------------------------------------------------
    std::string FieldInspector::numberText(int field, double si) const
    {
        const FieldInfo& f = doc_.info(field);
        if (f.kind == FieldKind::integer)
            return formatNumber(si, 0);
        if (f.quantity != Quantity::none)
            return units_.number(f.quantity, si, f.decimals);
        return formatNumber(si, f.decimals);
    }

    std::string FieldInspector::valueText(int field) const
    {
        const FieldInfo& f = doc_.info(field);
        const double v = doc_.get(field);
        if (numberKind(f))
            return numberText(field, v);
        return fieldText(f, v, units_);
    }

    std::string FieldInspector::unitText(int field) const
    {
        const FieldInfo& f = doc_.info(field);
        if (!numberKind(f))
            return {};
        return f.quantity != Quantity::none ? units_.unit(f.quantity).name : f.unit;
    }

    std::string FieldInspector::rangeText(int field) const
    {
        const FieldInfo& f = doc_.info(field);
        if (f.kind == FieldKind::choice)
            return std::to_string(f.choices.size()) + " choices";
        const std::string u = unitText(field);
        return numberText(field, f.min) + " to " + numberText(field, f.max) + (u.empty() ? "" : " " + u);
    }

    void FieldInspector::setValue(int field, double si)
    {
        doc_.beginGesture();
        doc_.set(field, si);
        doc_.endGesture();
    }

    void FieldInspector::toDefault(int field)
    {
        const FieldInfo& f = doc_.info(field);
        setValue(field, clampTo(f, f.def));
    }

    void FieldInspector::nudge(int field, int dir, bool fine)
    {
        const FieldInfo& f = doc_.info(field);
        const double v = doc_.get(field);
        switch (f.kind)
        {
            case FieldKind::toggle: doc_.set(field, v >= 0.5 ? 0.0 : 1.0); return;
            case FieldKind::choice:
            case FieldKind::integer: doc_.set(field, clampTo(f, v + dir)); return;
            case FieldKind::number:
                doc_.set(field, plainOf01(f, norm01(f, v) + static_cast<float>(dir) * (fine ? 0.001f : 0.01f)));
                return;
            default: return;
        }
    }

    bool FieldInspector::beginTyping(int field)
    {
        const FieldInfo& f = doc_.info(field);
        if (f.kind == FieldKind::toggle || f.table())
            return false;
        for (std::size_t i = 0; i < rows_.size(); ++i)
            if (rows_[i].field == field)
            {
                focusRow(static_cast<int>(i));
                editing_ = field;
                edit_.set(valueText(field), 24);
                refusal_.clear();
                refusedField_ = -1;
                filtering_ = false;
                return true;
            }
        return false;
    }

    // The typed text kept when it is a value within the field's range; else refused, the range said, the typing kept.
    void FieldInspector::commit()
    {
        if (editing_ < 0)
            return;
        const int field = editing_;
        const FieldInfo& f = doc_.info(field);
        std::string t = edit_.buffer;
        while (!t.empty() && t.back() == ' ')
            t.pop_back();
        double si = 0.0;
        bool ok = false;
        if (f.kind == FieldKind::choice)
        {
            ok = parseField(f, t, units_, si);
            if (!ok)
            {
                char* end = nullptr;
                const long n = std::strtol(t.c_str(), &end, 10);   // a choice by its number, counted from 1
                if (end != t.c_str() && *end == '\0' && n >= 1 && n <= static_cast<long>(f.choices.size()))
                {
                    si = f.min + static_cast<double>(n - 1);
                    ok = true;
                }
            }
            if (!ok)
            {
                std::string names;
                for (std::size_t k = 0; k < f.choices.size() && k < 6; ++k)
                    names += (k > 0 ? ", " : "") + f.choices[k];
                refusal_ = "One of: " + names + (f.choices.size() > 6 ? ", ..." : "");
                refusedField_ = field;
                return;
            }
        }
        else if (f.quantity != Quantity::none && f.kind == FieldKind::number)
            ok = units_.parse(f.quantity, t, si);
        else
        {
            char* end = nullptr;
            si = std::strtod(t.c_str(), &end);
            ok = end != t.c_str() && std::isfinite(si);
            if (ok)
            {
                std::string rest(end);
                while (!rest.empty() && rest.front() == ' ')
                    rest.erase(0, 1);
                ok = rest.empty() || lower(rest) == lower(f.unit);
            }
        }
        if (!ok)
        {
            refusal_ = "Not a number" + std::string(unitText(field).empty() ? "" : " of " + unitText(field)) + ": "
                     + rangeText(field);
            refusedField_ = field;
            return;
        }
        if (f.kind == FieldKind::integer && std::fabs(si - std::round(si)) > 1e-9)
        {
            refusal_ = "A whole number: " + rangeText(field);
            refusedField_ = field;
            return;
        }
        const double eps = 1e-9 * (1.0 + std::fabs(f.max - f.min));
        if (si < f.min - eps || si > f.max + eps)
        {
            refusal_ = "Out of range: " + rangeText(field);
            refusedField_ = field;
            return;
        }
        std::string why;
        if (!doc_.accepts(field, std::clamp(si, f.min, f.max), &why))
        {
            refusal_ = "Out of range: " + why;   // the document's own rule (BuildDoc: the host's least stated bore)
            refusedField_ = field;
            return;
        }
        setValue(field, std::clamp(si, f.min, f.max));
        editing_ = -1;
        refusal_.clear();
        refusedField_ = -1;
    }

    void FieldInspector::openMenu(int field, const Rect& at, funkgui::HostServices& h)
    {
        const FieldInfo& f = doc_.info(field);
        funkgui::MenuRequest m;
        const long now = std::lround(doc_.get(field) - f.min);
        for (std::size_t k = 0; k < f.choices.size(); ++k)
        {
            funkgui::MenuItem it;
            it.id = static_cast<int>(k) + 1;
            it.label = f.choices[k];
            it.radio = true;
            it.checked = static_cast<long>(k) == now;
            m.items.push_back(std::move(it));
        }
        m.anchor = at;
        m.selectedId = static_cast<int>(now) + 1;
        m.minWidth = at.w;
        h.showMenu(m, [this, field](int id) {
            if (id > 0)
                setValue(field, doc_.info(field).min + static_cast<double>(id - 1));
        });
    }

    // ---- time --------------------------------------------------------------------------------------------------------
    void FieldInspector::step(float dt)
    {
        if (doc_.revision() != docSeen_ && changedOnly_ && editing_ < 0 && !scrubbing_ && wheelField_ < 0)
            rebuild();                       // CHANGED holds what is changed now
        docSeen_ = doc_.revision();
        shownScroll_ = funkgui::ease::toward(shownScroll_, scroll_, dt, 0.06f, 0.25f);
        if (wheelField_ >= 0 && (wheelRest_ += std::max(dt, 0.0f)) >= 0.5f)
        {
            wheelField_ = -1;                // the wheel rested: its burst is one undo step
            doc_.endGesture();
        }
    }

    // ---- input -------------------------------------------------------------------------------------------------------
    void FieldInspector::pointerMove(const funkgui::PointerEvent& e) { hoverRow_ = rowAt({ e.x, e.y }); }

    funkgui::Cursor FieldInspector::cursorAt(funkgui::Point p) const
    {
        if (scrubbing_)
            return funkgui::Cursor::leftRight;
        if (filterRect().contains(p))
            return funkgui::Cursor::normal;
        const int i = rowAt(p);
        if (i < 0)
            return funkgui::Cursor::normal;
        const Row& r = rows_[static_cast<std::size_t>(i)];
        if (r.kind == Row::Kind::heading || r.kind == Row::Kind::table)
            return funkgui::Cursor::pointingHand;
        if (r.kind != Row::Kind::field)
            return funkgui::Cursor::normal;
        const FieldInfo& f = doc_.info(r.field);
        if (numberKind(f) && onScreen(r.value).contains(p))
            return funkgui::Cursor::leftRight;
        return onScreen(r.value).contains(p) ? funkgui::Cursor::pointingHand : funkgui::Cursor::normal;
    }

    void FieldInspector::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices& h)
    {
        const funkgui::Point p{ e.x, e.y };
        pressRow_ = -1;
        scrubbing_ = false;
        if (e.popup)
            return;
        if (filterRect().contains(p))
        {
            if (editing_ >= 0)
                commit();
            filtering_ = true;
            filter_.caret = static_cast<int>(filter_.buffer.size());
            return;
        }
        if (changedRect().contains(p))
        {
            setChangedOnly(!changedOnly_);
            return;
        }
        const int i = rowAt(p);
        if (editing_ >= 0 && (i < 0 || rows_[static_cast<std::size_t>(i)].field != editing_))
        {
            commit();                        // a press elsewhere keeps what was typed (or leaves it, refused)
            if (editing_ >= 0)
                editing_ = -1;
        }
        filtering_ = false;
        if (i < 0)
            return;
        const Row& r = rows_[static_cast<std::size_t>(i)];
        if (r.kind == Row::Kind::heading)
        {
            const std::string& t = filtered_[static_cast<std::size_t>(r.entry)].text;
            fold(t, !r.folded);
            return;
        }
        if (r.kind == Row::Kind::table)
        {
            if (onTable_)
                onTable_(filtered_[static_cast<std::size_t>(r.entry)]);
            return;
        }
        if (r.kind != Row::Kind::field)
            return;
        focus_ = i;
        const FieldInfo& f = doc_.info(r.field);
        const Rect value = onScreen(r.value);
        if (f.kind == FieldKind::toggle)
        {
            setValue(r.field, doc_.get(r.field) >= 0.5 ? 0.0 : 1.0);
            return;
        }
        if (f.kind == FieldKind::choice)
        {
            if (value.contains(p))
                openMenu(r.field, value, h);
            return;
        }
        if (!numberKind(f))
            return;
        if (value.contains(p))
        {
            pressRow_ = i;
            pressAt_ = p;
            press01_ = norm01(f, doc_.get(r.field));
        }
    }

    void FieldInspector::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (pressRow_ < 0)
            return;
        const Row& r = rows_[static_cast<std::size_t>(pressRow_)];
        const FieldInfo& f = doc_.info(r.field);
        const float dx = e.x - pressAt_.x;
        if (!scrubbing_)
        {
            if (std::fabs(dx) < 3.0f)
                return;
            if (editing_ >= 0)
                editing_ = -1;
            doc_.beginGesture();             // the drag is one undo step
            scrubbing_ = true;
        }
        const float fine = e.mods.shift ? 0.1f : 1.0f;
        const float v01 = press01_ + dx / 240.0f * fine;
        doc_.set(r.field, plainOf01(f, std::clamp(v01, 0.0f, 1.0f)));
    }

    void FieldInspector::pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (scrubbing_)
            doc_.endGesture();
        else if (pressRow_ >= 0)
        {
            const Row& r = rows_[static_cast<std::size_t>(pressRow_)];
            if (onScreen(r.value).contains({ e.x, e.y }))
                beginTyping(r.field);       // a click on a value types it
        }
        scrubbing_ = false;
        pressRow_ = -1;
    }

    void FieldInspector::doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        const int i = rowAt({ e.x, e.y });
        if (i < 0 || rows_[static_cast<std::size_t>(i)].kind != Row::Kind::field)
            return;
        const Row& r = rows_[static_cast<std::size_t>(i)];
        if (!onScreen(r.value).contains({ e.x, e.y }))
            return;
        if (doc_.info(r.field).kind == FieldKind::toggle)
            return;
        editing_ = -1;                       // the first click began typing; two are the default
        pressRow_ = -1;                      // and the second press's release types nothing
        refusal_.clear();
        toDefault(r.field);
    }

    bool FieldInspector::wheel(const funkgui::WheelEvent& e, funkgui::GestureController&, double)
    {
        if (e.dy == 0.0f)
            return false;
        const funkgui::Point p{ e.x, e.y };
        const int i = rowAt(p);
        if (i >= 0 && editing_ < 0)
        {
            const Row& r = rows_[static_cast<std::size_t>(i)];
            const bool on = r.kind == Row::Kind::field
                         && onScreen(r.value).contains(p);
            if (on && doc_.info(r.field).kind != FieldKind::toggle)
            {
                if (wheelField_ != r.field)
                {
                    if (wheelField_ >= 0)
                        doc_.endGesture();
                    doc_.beginGesture();
                    wheelField_ = r.field;
                }
                wheelRest_ = 0.0f;
                nudge(r.field, e.dy > 0.0f ? 1 : -1, e.mods.shift);
                return true;
            }
        }
        const float before = scroll_;
        scrollTo(scroll_ - e.dy * (e.smooth ? 40.0f : 72.0f));
        return scroll_ != before;
    }

    bool FieldInspector::key(const funkgui::KeyEvent& e, funkgui::GestureController& g)
    {
        using funkgui::Key;
        if (filtering_)
        {
            if (e.key == Key::escape)
            {
                if (filter_.buffer.empty())
                    filtering_ = false;
                else
                    setFilter({});
                return true;
            }
            if (e.key == Key::enter || e.key == Key::down)
            {
                filtering_ = false;
                for (std::size_t i = 0; i < rows_.size(); ++i)
                    if (rows_[i].kind == Row::Kind::field || rows_[i].kind == Row::Kind::table)
                    {
                        focusRow(static_cast<int>(i));
                        break;
                    }
                return true;
            }
            if (e.key == Key::tab)
            {
                filtering_ = false;
                return false;
            }
            const std::string before = filter_.buffer;
            filter_.key(e, 40);
            if (filter_.buffer != before)
            {
                scroll_ = shownScroll_ = 0.0f;
                rebuild();
            }
            return true;
        }
        if (editing_ >= 0)
        {
            if (e.key == Key::enter)
            {
                commit();
                return true;
            }
            if (e.key == Key::escape)
            {
                editing_ = -1;
                refusal_.clear();
                refusedField_ = -1;
                return true;
            }
            if (e.key == Key::tab)
            {
                commit();
                if (editing_ >= 0)
                    return true;             // refused: stays
                return false;
            }
            refusal_.clear();
            edit_.key(e, 24);
            return true;
        }
        const int field = focusedField();
        const auto move = [&](int dir) {
            int i = focus_;
            for (int k = 0; k < static_cast<int>(rows_.size()); ++k)
            {
                i = i < 0 ? (dir > 0 ? 0 : static_cast<int>(rows_.size()) - 1) : i + dir;
                if (i < 0 || i >= static_cast<int>(rows_.size()))
                    return;
                const Row::Kind kind = rows_[static_cast<std::size_t>(i)].kind;
                if (kind == Row::Kind::field || kind == Row::Kind::table || kind == Row::Kind::heading)
                {
                    focusRow(i);
                    return;
                }
            }
        };
        // A heading folds (Enter, Space, Left folds, Right opens); a table opens (Enter, Space).
        const Row* at = focus_ >= 0 && focus_ < static_cast<int>(rows_.size()) ? &rows_[static_cast<std::size_t>(focus_)] : nullptr;
        if (at != nullptr && at->kind == Row::Kind::heading)
        {
            const std::string heading = filtered_[static_cast<std::size_t>(at->entry)].text;
            const bool folded = at->folded;
            if (e.key == Key::enter || e.key == Key::space || (e.key == Key::left && !folded) || (e.key == Key::right && folded))
            {
                fold(heading, !folded);
                return true;
            }
        }
        if (at != nullptr && at->kind == Row::Kind::table && (e.key == Key::enter || e.key == Key::space))
        {
            if (onTable_)
                onTable_(filtered_[static_cast<std::size_t>(at->entry)]);
            return true;
        }
        switch (e.key)
        {
            case Key::down: move(1); return true;
            case Key::up: move(-1); return true;
            case Key::enter:
                if (field < 0)
                    return false;
                if (doc_.info(field).kind == FieldKind::choice)
                {
                    Rect row, value;
                    fieldRect(field, row, value);
                    openMenu(field, value, g.host());
                }
                else if (doc_.info(field).kind == FieldKind::toggle)
                    setValue(field, doc_.get(field) >= 0.5 ? 0.0 : 1.0);
                else
                    beginTyping(field);
                return true;
            case Key::space:
                if (field >= 0 && doc_.info(field).kind == FieldKind::toggle)
                {
                    setValue(field, doc_.get(field) >= 0.5 ? 0.0 : 1.0);
                    return true;
                }
                return field >= 0;
            case Key::left:
            case Key::right:
                if (field < 0)
                    return false;
                doc_.beginGesture();
                nudge(field, e.key == Key::right ? 1 : -1, e.mods.shift);
                doc_.endGesture();
                return true;
            case Key::del:
            case Key::backspace:
                if (field < 0)
                    return false;
                toDefault(field);
                return true;
            case Key::escape:
                if (!filter_.buffer.empty() || changedOnly_)
                {
                    setFilter({});
                    setChangedOnly(false);
                    return true;
                }
                return false;
            case Key::character:
                if (e.ch == '/' && !card_)
                {
                    filtering_ = true;
                    return true;
                }
                if (field >= 0 && ((e.ch >= '0' && e.ch <= '9') || e.ch == '-' || e.ch == '.') && beginTyping(field))
                {
                    edit_.set("", 24);
                    edit_.key(e, 24);
                    return true;
                }
                return false;
            default: return false;
        }
    }

    void FieldInspector::focusChanged(bool focused)
    {
        if (focused)
            return;
        if (editing_ >= 0)
        {
            commit();
            editing_ = -1;
        }
        filtering_ = false;
    }

    // ---- drawing -----------------------------------------------------------------------------------------------------
    void FieldInspector::drawHeader(Canvas& c, const Look& l) const
    {
        if (card_)
        {
            textAt(c, upper(cardTitle_), bounds_.x + kPad + 4.0f, bounds_.y + 17.0f, kHeadText, l.value);
            c.hairlineH(bounds_.x + 4.0f, c.snapY(bounds_.y + 33.0f), bounds_.w - 8.0f, l.edge);
            if (!cardMore_.empty())
                textAt(c, cardMore_, bounds_.x + kPad + 4.0f, bounds_.bottom() - 13.0f, kWidgetSmall, l.label);
            return;
        }
        const Rect fr = filterRect();
        c.rrect(fr.x, fr.y, fr.w, fr.h, 5.0f, filtering_ ? l.ground : l.face, 1.0f, filtering_ ? l.text : l.edge);
        // a magnifier
        const float mx = fr.x + 13.0f, my = fr.centreY() - 1.0f;
        c.disc(mx, my, 4.5f, kClear, 1.3f, l.label);
        c.segment(mx + 3.2f, my + 3.2f, mx + 6.5f, my + 6.5f, 1.5f, l.label);
        const float tx = fr.x + 26.0f;
        if (filter_.buffer.empty() && !filtering_)
            textAt(c, "Find a field", tx, fr.centreY(), kValueText, l.dim);
        else
        {
            textAt(c, filter_.buffer, tx, fr.centreY(), kValueText, l.value);
            if (filtering_)
            {
                const float cx = tx + c.textWidth(filter_.buffer.substr(0, static_cast<std::size_t>(filter_.caret)).c_str(), kValueText);
                c.segment(cx, fr.y + 6.0f, cx, fr.bottom() - 6.0f, 1.5f, l.accent);
            }
        }
        const Rect cr = changedRect();
        int changedCount = 0;
        for (const int f : searchable_.empty() ? fieldsOf(outline_) : searchable_)
            changedCount += changed(f) ? 1 : 0;
        c.rrect(cr.x, cr.y, cr.w, cr.h, 5.0f, changedOnly_ ? l.accentDim : l.face, 1.0f, changedOnly_ ? l.accent : l.edge);
        c.disc(cr.x + 12.0f, cr.centreY(), 3.0f, changedCount > 0 ? l.accent : l.dim);
        textAt(c, "CHANGED " + std::to_string(changedCount), cr.x + 21.0f, cr.centreY(), kWidgetLabel,
               changedOnly_ ? l.value : l.text);
        std::string line;
        const int shown = shownFields();
        if (!filter_.buffer.empty())
            line = shown == 0 ? "NO FIELD MATCHES" : std::to_string(shown) + (shown == 1 ? " FIELD" : " FIELDS") + " MATCH, IN EVERY SECTION";
        else if (changedOnly_)
            line = std::to_string(shown) + " CHANGED FROM THEIR DEFAULTS";
        else
            line = std::to_string(shown) + " FIELDS   TYPE, DRAG OR WHEEL A VALUE";
        textAt(c, fitText(c, line, kWidgetSmall, bounds_.w - 2.0f * kPad), bounds_.x + kPad, bounds_.y + 45.0f, kWidgetSmall, l.dim);
    }

    void FieldInspector::drawHeading(Canvas& c, const Look& l, const Row& r, const Rect& at) const
    {
        const OutlineEntry& e = filtered_[static_cast<std::size_t>(r.entry)];
        const float cy = at.centreY() + 1.0f, x = at.x + kPad;
        // a chevron: down when open, right when folded
        if (r.folded)
        {
            c.segment(x + 2.0f, cy - 4.0f, x + 6.0f, cy, 1.4f, l.label);
            c.segment(x + 6.0f, cy, x + 2.0f, cy + 4.0f, 1.4f, l.label);
        }
        else
        {
            c.segment(x, cy - 2.0f, x + 4.0f, cy + 2.0f, 1.4f, l.label);
            c.segment(x + 4.0f, cy + 2.0f, x + 8.0f, cy - 2.0f, 1.4f, l.label);
        }
        textAt(c, upper(e.text), x + 16.0f, cy, kHeadText, l.text);
        // how many under it are changed
        int changedHere = 0;
        for (std::size_t k = static_cast<std::size_t>(r.entry) + 1; k < filtered_.size(); ++k)
        {
            const OutlineEntry& o = filtered_[k];
            if (o.kind == OutlineEntry::Kind::heading)
                break;
            changedHere += o.kind == OutlineEntry::Kind::field && changed(o.field) ? 1 : 0;
        }
        if (changedHere > 0)
            textAt(c, std::to_string(changedHere) + " CHANGED", at.right() - kPad, cy, kWidgetSmall, l.accent, Align::right);
        c.hairlineH(at.x + kPad, c.snapY(at.bottom() - 1.0f), at.w - 2.0f * kPad, l.edge);
    }

    void FieldInspector::drawValueBox(Canvas& c, const Look& l, const Row& r, const Rect& box, bool hot) const
    {
        const FieldInfo& f = doc_.info(r.field);
        const bool typing = editing_ == r.field;
        const bool refused = refusedField_ == r.field && !refusal_.empty();
        if (f.kind == FieldKind::toggle)
        {
            const bool on = doc_.get(r.field) >= 0.5;
            c.rrect(box.x, box.y, box.w, box.h, box.h * 0.5f, on ? l.accentDim : l.face, 1.0f, on ? l.accent : l.edge);
            const float k = box.h - 6.0f;
            c.disc(on ? box.right() - 3.0f - k * 0.5f : box.x + 3.0f + k * 0.5f, box.centreY(), k * 0.5f, on ? l.value : l.text);
            return;
        }
        const Col edge = refused ? l.error : typing ? l.text : hot ? l.dim : l.edge;
        c.rrect(box.x, box.y, box.w, box.h, 4.0f, typing ? l.ground : (hot ? l.faceHi : l.face), 1.0f, edge);
        const Canvas::ClipScope clip(c, box.reduced(1.0f));
        if (typing)
        {
            const std::string& s = edit_.buffer;
            const float tx = box.x + 8.0f;
            if (edit_.allSelected && !s.empty())
                c.rrect(tx - 2.0f, box.y + 4.0f, c.textWidth(s.c_str(), kValueText) + 4.0f, box.h - 8.0f, 2.0f, l.accentDim);
            textAt(c, s, tx, box.centreY(), kValueText, l.value);
            const float cx = tx + c.textWidth(s.substr(0, static_cast<std::size_t>(std::clamp<int>(edit_.caret, 0, static_cast<int>(s.size())))).c_str(), kValueText);
            c.segment(cx, box.y + 5.0f, cx, box.bottom() - 5.0f, 1.5f, l.accent);
            return;
        }
        if (f.kind == FieldKind::choice)
        {
            textAt(c, fitText(c, valueText(r.field), kValueText, box.w - 28.0f), box.x + 8.0f, box.centreY(), kValueText, l.value);
            const float cx = box.right() - 11.0f, cy = box.centreY();
            c.segment(cx - 3.5f, cy - 1.5f, cx, cy + 2.0f, 1.3f, l.label);
            c.segment(cx, cy + 2.0f, cx + 3.5f, cy - 1.5f, 1.3f, l.label);
            return;
        }
        // a number, right-aligned, its unit after it in the label's ink
        const std::string unit = unitText(r.field);
        const float uw = unit.empty() ? 0.0f : c.textWidth(unit.c_str(), kWidgetSmall) + 6.0f;
        textAt(c, valueText(r.field), box.right() - 8.0f - uw, box.centreY(), kValueText, l.value, Align::right);
        if (!unit.empty())
            textAt(c, unit, box.right() - 8.0f, box.centreY(), kWidgetSmall, l.label, Align::right);
    }

    void FieldInspector::drawRows(Canvas& c, const Look& l) const
    {
        const Rect content = contentRect();
        const bool compact = style_ == Style::list;
        for (std::size_t i = 0; i < rows_.size(); ++i)
        {
            const Row& r = rows_[i];
            const Rect at = onScreen(r.rect);
            if (at.bottom() < content.y || at.y > content.bottom())
                continue;
            const bool hot = static_cast<int>(i) == hoverRow_;
            switch (r.kind)
            {
                case Row::Kind::heading:
                    if (static_cast<int>(i) == focus_)
                        c.rrect(at.x + 4.0f, at.y + 1.0f, at.w - 8.0f, at.h - 2.0f, 4.0f, l.over(l.edge, 0.55f));
                    drawHeading(c, l, r, at);
                    break;
                case Row::Kind::note:
                {
                    const OutlineEntry& e = filtered_[static_cast<std::size_t>(r.entry)];
                    const std::size_t chars = static_cast<std::size_t>((at.w - 2.0f * kPad) / (10.0f * kWrap));
                    float y = at.y + 4.0f;
                    for (const std::string& line : wrap(e.text, chars))
                    {
                        c.text(line.c_str(), at.x + kPad, y, kWidgetSmall, l.accent);
                        y += 13.0f;
                    }
                    break;
                }
                case Row::Kind::table:
                {
                    const OutlineEntry& e = filtered_[static_cast<std::size_t>(r.entry)];
                    if (hot || static_cast<int>(i) == focus_)
                        c.rrect(at.x + 4.0f, at.y + 1.0f, at.w - 8.0f, at.h - 2.0f, 4.0f,
                                static_cast<int>(i) == focus_ ? l.over(l.edge, 0.55f) : l.over(l.edge, 0.35f));
                    std::string what;
                    if (e.widget == "array")
                        what = std::to_string(e.fields.size()) + " VALUES";
                    else if (e.widget == "map" && !e.fields.empty())
                    {
                        const Map16 m = doc_.map(e.fields[0]);
                        what = "MAP " + std::to_string(m.nx) + " X " + std::to_string(m.ny);
                    }
                    else if (!e.fields.empty())
                        what = "CURVE " + std::to_string(doc_.curve(e.fields[0]).n) + " POINTS";
                    textAt(c, fitText(c, e.text, kLabelText, at.w - 190.0f), at.x + 14.0f, at.centreY(), kLabelText, l.text);
                    const Rect v = onScreen(r.value);
                    textAt(c, what, v.x - 10.0f, at.centreY(), kWidgetSmall, l.label, Align::right);
                    c.rrect(v.x, v.y, v.w, v.h, 4.0f, hot ? l.faceHi : l.face, 1.0f, l.edge);
                    textAt(c, "OPEN", v.centreX(), v.centreY(), kWidgetLabel, l.text, Align::centre);
                    break;
                }
                case Row::Kind::field:
                {
                    const FieldInfo& f = doc_.info(r.field);
                    const bool focused = static_cast<int>(i) == focus_;
                    if (focused || hot)
                        c.rrect(at.x + 4.0f, at.y + 1.0f, at.w - 8.0f, at.h - 2.0f, 4.0f,
                                focused ? l.over(l.edge, 0.55f) : l.over(l.edge, 0.3f));
                    if (changed(r.field))
                        c.disc(at.x + 8.0f, compact || !numberKind(f) ? at.centreY() : at.y + 16.0f, 2.6f, l.accent);
                    const Rect v = onScreen(r.value);
                    const float labelY = compact || !numberKind(f) ? at.centreY() : at.y + 16.0f;
                    const std::string label = f.label.empty() ? f.path : f.label;
                    textAt(c, fitText(c, label, kLabelText, v.x - at.x - 26.0f), at.x + 14.0f, labelY, kLabelText,
                           changed(r.field) ? l.value : l.text);
                    drawValueBox(c, l, r, v, hot);
                    if (!compact && numberKind(f))
                    {
                        // where it sits in its range, a tick at its default; or what a refusal said
                        const float x0 = at.x + 14.0f, x1 = v.x - 16.0f, y = at.y + 32.0f;
                        if (refusedField_ == r.field && !refusal_.empty())
                            textAt(c, fitText(c, refusal_, kWidgetSmall, at.w - 28.0f), x0, y, kWidgetSmall, l.error);
                        else if (x1 > x0 + 20.0f)
                        {
                            const float t = norm01(f, doc_.get(r.field)), td = norm01(f, clampTo(f, f.def));
                            c.segment(x0, y, x1, y, 2.0f, l.edge);
                            c.segment(x0, y, x0 + (x1 - x0) * t, y, 2.0f, l.dim);
                            const float dx = x0 + (x1 - x0) * td;
                            c.segment(dx, y - 4.0f, dx, y + 4.0f, 1.0f, l.label);
                            c.disc(x0 + (x1 - x0) * t, y, 3.2f, changed(r.field) ? l.value : l.text);
                        }
                    }
                    else if (refusedField_ == r.field && !refusal_.empty())
                        textAt(c, fitText(c, refusal_, kWidgetSmall, v.x - at.x - 26.0f), at.x + 14.0f, at.bottom() - 6.0f,
                               kWidgetSmall, l.error);
                    break;
                }
                case Row::Kind::empty: break;
            }
        }
    }

    void FieldInspector::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        if (card_)
        {
            // a card beside the part: a shadow, a face, an edge
            c.rrect(bounds_.x + 3.0f, bounds_.y + 5.0f, bounds_.w, bounds_.h, 9.0f, l.over(funkgui::Col{ 0, 0, 0, 255 }, 0.5f));
            c.rrect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, 8.0f, l.over(l.edge, 0.35f), 1.0f, l.dim);
        }
        drawHeader(c, l);
        const Rect content = contentRect();
        {
            const Canvas::ClipScope clip(c, { c.snapX(content.x), c.snapY(content.y), content.w, content.h });
            drawRows(c, l);
            // The heading of what is at the top stays there while its group scrolls under it (pushed up by the next).
            int top = -1;
            for (std::size_t i = 0; i < rows_.size(); ++i)
                if (rows_[i].kind == Row::Kind::heading && rows_[i].rect.y <= shownScroll_ + 0.5f)
                    top = static_cast<int>(i);
            if (top >= 0 && shownScroll_ > 0.5f)
            {
                const Row& h = rows_[static_cast<std::size_t>(top)];
                float y = content.y;
                for (std::size_t k = static_cast<std::size_t>(top) + 1; k < rows_.size(); ++k)
                    if (rows_[k].kind == Row::Kind::heading)
                    {
                        y = std::min(y, onScreen(rows_[k].rect).y - h.rect.h);
                        break;
                    }
                const Rect at{ h.rect.x, y, h.rect.w, h.rect.h };
                c.rrect(at.x, at.y, at.w, at.h, 0.0f, card_ ? l.over(l.edge, 0.35f) : l.ground);
                drawHeading(c, l, h, at);
            }
        }
        // a thumb when it scrolls
        if (contentH_ > content.h + 1.0f && content.h > 0.0f)
        {
            const float th_ = std::max(24.0f, content.h * content.h / contentH_);
            const float ty = content.y + (content.h - th_) * (shownScroll_ / (contentH_ - content.h));
            c.rrect(bounds_.right() - 4.0f, ty + 2.0f, 3.0f, th_ - 4.0f, 1.5f, l.dim);
        }
        if (focusRing && focus_ < 0 && !filtering_)
            ring(c, l, bounds_.reduced(2.0f), 6.0f);
    }

    // ---- accessibility and help --------------------------------------------------------------------------------------
    void FieldInspector::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem list;
        list.id = id_;
        list.role = funkgui::a11yrole::list;
        list.bounds = bounds_;
        list.title = card_ ? cardTitle_ : "Fields";
        list.value = std::to_string(shownFields()) + " fields";
        out.push_back(list);
        const Rect content = contentRect();
        int k = 0;
        for (const Row& r : rows_)
        {
            if (r.kind != Row::Kind::field || k >= kIds)
                continue;
            const Rect at = onScreen(r.rect);
            if (at.bottom() <= content.y || at.y >= content.bottom())
                continue;
            const FieldInfo& f = doc_.info(r.field);
            funkgui::A11yItem it;
            it.id = id_ + 1 + static_cast<uint32_t>(k++);
            it.parent = id_;
            it.bounds = at;
            it.title = f.label.empty() ? f.path : f.label;
            it.help = f.help;
            it.value = valueText(r.field) + (unitText(r.field).empty() ? "" : " " + unitText(r.field));
            if (f.kind == FieldKind::toggle)
            {
                it.role = funkgui::A11yRole::toggleButton;
                it.checkable = true;
                it.checked = doc_.get(r.field) >= 0.5;
            }
            else if (f.kind == FieldKind::choice)
                it.role = funkgui::A11yRole::comboBox;
            else
            {
                it.role = funkgui::A11yRole::slider;
                it.v = norm01(f, doc_.get(r.field));
                it.step = 0.01;
            }
            out.push_back(std::move(it));
        }
    }

    bool FieldInspector::a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController&)
    {
        if (id <= id_)
            return false;
        const Rect content = contentRect();
        int k = 0;
        for (const Row& r : rows_)
        {
            if (r.kind != Row::Kind::field)
                continue;
            const Rect at = onScreen(r.rect);
            if (at.bottom() <= content.y || at.y >= content.bottom())
                continue;
            if (id_ + 1 + static_cast<uint32_t>(k++) != id)
                continue;
            const FieldInfo& f = doc_.info(r.field);
            switch (a)
            {
                case funkgui::A11yAction::increment:
                case funkgui::A11yAction::decrement:
                    doc_.beginGesture();
                    nudge(r.field, a == funkgui::A11yAction::increment ? 1 : -1, false);
                    doc_.endGesture();
                    return true;
                case funkgui::A11yAction::setValue: setValue(r.field, plainOf01(f, static_cast<float>(v))); return true;
                case funkgui::A11yAction::toggle:
                case funkgui::A11yAction::press:
                    if (f.kind == FieldKind::toggle)
                        setValue(r.field, doc_.get(r.field) >= 0.5 ? 0.0 : 1.0);
                    else
                        beginTyping(r.field);
                    return true;
                default: return false;
            }
        }
        return false;
    }

    int FieldInspector::helpPart() const
    {
        const int row = hoverRow_ >= 0 ? hoverRow_ : focus_;
        return row + 1;
    }

    bool FieldInspector::help(std::string& title, std::string& body, Rect& anchor) const
    {
        const int row = hoverRow_ >= 0 ? hoverRow_ : focus_;
        if (row < 0 || row >= static_cast<int>(rows_.size()) || rows_[static_cast<std::size_t>(row)].kind != Row::Kind::field
            || editing_ >= 0)
            return false;
        const Row& r = rows_[static_cast<std::size_t>(row)];
        const FieldInfo& f = doc_.info(r.field);
        title = f.label.empty() ? f.path : f.label;
        std::string facts;
        if (f.kind == FieldKind::choice || numberKind(f))
            facts = "Range " + rangeText(r.field) + ", default " + (numberKind(f) ? numberText(r.field, clampTo(f, f.def)) + (unitText(r.field).empty() ? "" : " " + unitText(r.field))
                                                                                       : fieldText(f, f.def, units_)) + ".";
        else
            facts = std::string("Default ") + (f.def >= 0.5 ? "on." : "off.");
        // The help and the facts as two sentences: a full stop after the help when it has none.
        std::string help = f.help;
        while (!help.empty() && help.back() == ' ')
            help.pop_back();
        if (!help.empty() && help.back() != '.' && help.back() != '!' && help.back() != '?' && help.back() != ':')
            help += '.';
        body = help.empty() ? facts : help + " " + facts;
        anchor = onScreen(r.rect);
        return true;
    }

    uint64_t FieldInspector::drawKey() const
    {
        return DrawKey{}.mix(doc_.revision()).mix(scroll_).mix(focus_).mix(hoverRow_).mix(editing_).mix(edit_.buffer)
            .mix(edit_.caret).mix(filter_.buffer).mix(changedOnly_).mix(refusal_).mix(filtering_).h;
    }
}
