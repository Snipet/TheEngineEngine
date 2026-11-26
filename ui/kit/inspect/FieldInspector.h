// ui/kit/inspect/FieldInspector.h: an inspector's column drawn for a workshop rather than a synthesiser (the owner,
// 25 Nov: "the right-side controls ... are relatively clunky"; the owner chose Rows). One widget, two looks, the same
// behaviour:
//
//   Style::rows    the inspector (RowsPage, the frame's pane): one field a row: its label, its value as a number and
//                  unit, a thin bar of where it sits in its range with a tick at its default; groups fold; a filter
//                  box finds a field by name across the workspace; a mark on each field moved off its default, and
//                  CHANGED shows those alone.
//   Style::list    compact rows: the card (setCard): the few fields of a part chosen on the stage, beside it.
//
// Every value: a click on it types it (a number with or without its unit, in the user's units: "86", "86 mm",
// "3.39 in"; a choice by its name or number), Enter keeps it when it is within the field's range and refuses it
// otherwise, saying the range (it is never clamped); a drag sideways scrubs it (240 px the whole travel, Shift ten
// times finer); the wheel nudges it (a hundredth of the travel, Shift a thousandth; a choice or an integer a step);
// a double-click puts it back to its default. Each edit is one undo step (FieldDoc gestures).
//
// Keyboard: the widget is one Tab stop; Up and Down move between fields, Enter or a digit types the one focused,
// Left and Right nudge it (a choice: the next or the one before), Space flips a switch, Delete puts it to its
// default, / goes to the filter, Escape leaves typing or clears the filter. Hover help: each row's (the field's
// ui::help, and its range and default), keyed by row (Item::helpPart). Accessibility: a list of the rows shown, each a
// slider, a switch or a menu with its value and actions.
#pragma once

#include "kit/bind/Fields.h"
#include "kit/bind/Units.h"
#include "kit/inspect/Outline.h"
#include "kit/widgets/Widget.h"

#include <funkgui/text/LineEdit.h>

#include <functional>
#include <string>
#include <vector>

namespace tee::ui
{
    class FieldInspector final : public Widget
    {
    public:
        enum class Style : std::uint8_t { rows, list };
        // The row the wheel counts in: a notch of a notched wheel scrolls three (funkgui::scroll::wheelPx, as every
        // list does); a smooth delta scrolls its own px.
        static constexpr float kWheelRowH = 36.0f;

        FieldInspector(uint32_t id, FieldDoc& doc, const Units& units, Style style);

        // What it shows: the page's outline; and, while the filter holds text, the fields it searches (the
        // workspace's: every section it shows; none: the outline's own).
        void setOutline(Outline outline);
        const Outline& outline() const noexcept { return outline_; }
        void setSearchable(std::vector<int> fields);
        // The card's look (design 3): a title, a frame and a shadow, no filter; `more` names what the column holds.
        void setCard(std::string title, std::string more);
        // A table's row was pressed (the frame opens it on a page of its own).
        void setOnTable(std::function<void(const OutlineEntry&)> f) { onTable_ = std::move(f); }

        void setFilter(std::string text);
        void setChangedOnly(bool on);
        void fold(const std::string& heading, bool folded);
        const std::string& filter() const noexcept { return filter_.buffer; }
        bool changedOnly() const noexcept { return changedOnly_; }

        // For tests and probes: the rows as laid out (in the widget's px, before the scroll), the field focused, the
        // value being typed and what a refusal said.
        struct Row
        {
            enum class Kind : std::uint8_t { heading, field, table, note, empty };
            Kind kind = Kind::field;
            int entry = -1;                  // in the shown outline
            int field = -1;
            funkgui::Rect rect{}, value{};   // the row; its value's box (an OPEN chip for a table)
            bool folded = false;
            float said = 0.0f;               // the height its refusal's sentence adds (wrapped, never cut)
        };
        const std::vector<Row>& rows() const noexcept { return rows_; }
        // A field's row on screen (scrolled), its value's box, and whether it is shown now.
        bool fieldRect(int field, funkgui::Rect& row, funkgui::Rect& value) const;
        funkgui::Rect filterRect() const noexcept;
        funkgui::Rect changedRect() const noexcept;
        int focusedField() const noexcept;
        int focusedRow() const noexcept { return focus_; }   // a row of rows(): a field's, a heading's or a table's
        bool typing() const noexcept { return editing_ >= 0; }
        const std::string& typed() const noexcept { return edit_.buffer; }
        const std::string& refusal() const noexcept { return refusal_; }
        // The refusal's sentence as drawn: wrapped to the row's width, every word kept.
        std::vector<std::string> refusalShown() const;
        float scroll() const noexcept { return scroll_; }
        float contentHeight() const noexcept { return contentH_; }
        int shownFields() const noexcept;    // fields the rows hold now (filter and CHANGED applied)
        // Begins typing a field's value (as a click on it does): false when it is not shown.
        bool beginTyping(int field);
        // The focus on the first field or table (a page just shown); false when there is none.
        bool focusFirst();
        // The focus on the table row whose fields include `field` (back from the table's page); false when none.
        bool focusTable(int field);
        // The text a field's value shows (its number, or a choice's name, or ON / OFF) and its unit.
        std::string valueText(int field) const;
        std::string unitText(int field) const;

        // Item
        void setPlace(const funkgui::Rect& r, bool shown) override;
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point p) const override;
        void pointerMove(const funkgui::PointerEvent& e) override;
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices& h) override;
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        bool wheel(const funkgui::WheelEvent& e, funkgui::GestureController& g, double now) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController& g) override;
        bool takesAllKeys() const override { return editing_ >= 0 || filtering_; }
        void focusChanged(bool focused) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController& g) override;
        bool help(std::string& title, std::string& body, funkgui::Rect& anchor) const override;
        int helpPart() const override;
        // A controller's command (EditCommand.h), through what the keys and the wheel already do: rowUp/rowDown move
        // the focus (headings and tables too, the view following); value* nudge the focused field as the wheel does
        // (Coarse: ten nudges; fine: a tenth; a burst one undo step until it rests 0.5 s; a value the document refuses
        // says why under its row); activate flips a switch, folds a group, steps a choice, opens a table (a number:
        // false); toDefault; changedOnly (the focus kept on its field); back lets the focus go. Pages, workspaces,
        // undo and redo: false (the frame's).
        bool edit(EditCommand cmd, bool fine) override;
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return shownScroll_ == scroll_; }
        const char* kind() const override { return "inspector"; }

    private:
        void layout();
        funkgui::Rect contentRect() const noexcept;
        float headerH() const noexcept;
        int rowAt(funkgui::Point p) const;   // the row under p (in widget px, scrolled), -1: none
        funkgui::Rect onScreen(const funkgui::Rect& r) const noexcept;
        bool matches(int field) const;
        bool changed(int field) const;
        const Outline& shownOutline() const noexcept { return filtered_; }
        void rebuild();                      // the shown outline from the filter and CHANGED, then the layout
        void commit();
        bool nudge(int field, int dir, bool fine);   // false: nothing moved (at an end, or refused: said)
        void setValue(int field, double si);  // one undo step
        void toDefault(int field);
        void openMenu(int field, const funkgui::Rect& at, funkgui::HostServices& h);
        void keepInView(int row);
        void scrollTo(float s);
        void focusRow(int row);
        bool moveFocus(int dir);             // to the next field, table or heading; false at the end
        std::string rangeText(int field) const;
        std::string numberText(int field, double si) const;

        void drawRows(funkgui::Canvas& c, const Look& l) const;
        void drawValueBox(funkgui::Canvas& c, const Look& l, const Row& r, const funkgui::Rect& box, bool hot) const;
        void drawHeading(funkgui::Canvas& c, const Look& l, const Row& r, const funkgui::Rect& at) const;
        void drawHeader(funkgui::Canvas& c, const Look& l) const;

        FieldDoc& doc_;
        const Units& units_;
        Style style_;
        Outline outline_, filtered_;
        std::vector<int> searchable_;
        std::vector<std::string> folded_;
        std::vector<Row> rows_;
        std::string cardTitle_, cardMore_;
        bool card_ = false;
        std::function<void(const OutlineEntry&)> onTable_;
        funkgui::text::LineEdit filter_;
        bool filtering_ = false, changedOnly_ = false;
        float scroll_ = 0.0f, shownScroll_ = 0.0f, contentH_ = 0.0f;
        int focus_ = -1;                     // a row (a field's)
        int hoverRow_ = -1;
        // typing a value
        int editing_ = -1;                   // the field typed, -1: none
        funkgui::text::LineEdit edit_;
        std::string refusal_;
        int refusedField_ = -1;
        // a press: scrub (sideways on a value) or a click
        int pressRow_ = -1;
        funkgui::Point pressAt_{};
        float press01_ = 0.0f;
        bool scrubbing_ = false;
        // the wheel's burst: one undo step until it rests
        int wheelField_ = -1;
        float wheelRest_ = 0.0f;
        std::uint64_t docSeen_ = 0;
        // the refusal the rows were laid out for (a refused row grows to hold its sentence)
        int refusalLaidField_ = -1;
        std::string refusalLaid_;
        std::vector<std::string> refusalLines(float width) const;
    };
}
