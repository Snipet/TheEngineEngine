// ui/kit/inspect/RowsPage.cpp: see RowsPage.h.
#include "kit/inspect/RowsPage.h"

#include <algorithm>

namespace tee::ui
{
    std::unique_ptr<RowsPage> RowsPage::over(const FieldPage& page, FieldDoc& doc, const Units& units)
    {
        Outline outline = outlineOf(page, doc);
        int tables = 0, rest = 0;
        for (const OutlineEntry& e : outline)
            if (e.kind == OutlineEntry::Kind::table)
                ++tables;
            else if (e.kind == OutlineEntry::Kind::field)
                ++rest;
        if (rest == 0 && tables <= 1)
            return nullptr;                  // a table alone (or nothing of the Spec): the page stays as it is
        return std::make_unique<RowsPage>(doc, units, std::move(outline), page.width(), page.height());
    }

    RowsPage::RowsPage(FieldDoc& doc, const Units& units, Outline outline, int width, int height)
        : Board(width, height, 1, 1)
    {
        setTabWraps(false);                  // Tab past the rows: the frame's turn
        for (const OutlineEntry& e : outline)
            if (e.kind == OutlineEntry::Kind::table)
                tables_.push_back({ e.text, e.fields });
        fields_ = fieldsOf(outline);
        rows_ = &make<FieldInspector>(doc, units, FieldInspector::Style::rows);
        rows_->setOutline(std::move(outline));
        rows_->setOnTable([this](const OutlineEntry& e) {
            if (onTable_)
                onTable_({ e.text, e.fields });
        });
    }

    void RowsPage::place(int width, int height, const funkgui::Rect& room)
    {
        room_ = room;
        if (width != this->width() || height != this->height())
            resize(width, height);           // arrange() places the rows
        else
            arrange();
    }

    void RowsPage::arrange()
    {
        rows_->setPlace(room_, !room_.isEmpty());
        bump();
    }

    bool RowsPage::key(const funkgui::KeyEvent& e)
    {
        if (e.key == funkgui::Key::escape && focus() >= 0 && gestures() != nullptr && rows_->key(e, *gestures()))
            return true;
        return Board::key(e);
    }

    uint64_t RowsPage::drawKey() const { return DrawKey{}.mix(rows_->drawKey()).mix(tooltip().drawKey()).h; }
}
