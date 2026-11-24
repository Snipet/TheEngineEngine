// ui/kit/widgets/Tables.h: the editors of the Spec's two table shapes (docs/plan/02-contracts.md section 2, rule 6:
// spec/tables.fm's Curve16 and Map16), card J2 item 6.
//
//   CurveEdit   a Curve16 as points on a Plot: drag a point (its x kept between its neighbours'), arrows choose and
//               nudge, the plot's pan and zoom elsewhere.
//   MapEdit     a Map16 as a table and a heat map: a cell is set by typing (a digit starts, Enter keeps, Escape drops)
//               or by dragging up and down on it; the axes' values are the header row and column, typed the same
//               way; the operating point lights its cell (live()). A block of cells is chosen with Shift and a click
//               or the arrows, from the cell chosen last (block()); its owner acts on it (a nudge, smoothing).
//
// What they edit is a CurveSource or a MapSource (ui/kit/bind/ adapts a Spec field of the document to one): read each
// tick, written whole, each edit (a drag, a typed value, a key's nudge) bracketed by beginEdit and endEdit, one step of
// undo. Values are in the field's own units (SI); the owner gives the text both ways (the user's units: Units.h).
#pragma once

#include "kit/widgets/Plot.h"

#include <funkgui/text/LineEdit.h>

#include <array>
#include <functional>
#include <string>
#include <string_view>

namespace tee::ui
{
    inline constexpr int kTableMax = 16;

    struct Curve16
    {
        int n = 0;                                   // points in use, 2 to 16
        std::array<double, kTableMax> x{}, y{};      // x rising
        bool operator==(const Curve16&) const = default;
    };

    struct Map16
    {
        int nx = 0, ny = 0;                          // columns and rows in use
        std::array<double, kTableMax> x{}, y{};      // the axes, rising
        std::array<std::array<double, kTableMax>, kTableMax> z{};   // z[row iy][column ix]
        bool operator==(const Map16&) const = default;
    };

    struct CurveSource
    {
        virtual ~CurveSource() = default;
        virtual Curve16 curve() const = 0;
        virtual void setCurve(const Curve16&) = 0;
        virtual void beginEdit() = 0;
        virtual void endEdit() = 0;
    };

    struct MapSource
    {
        virtual ~MapSource() = default;
        virtual Map16 map() const = 0;
        virtual void setMap(const Map16&) = 0;
        virtual void beginEdit() = 0;
        virtual void endEdit() = 0;
    };

    // A value's text and its reading back (to the field's units; false: not a number of ours).
    struct TableText
    {
        std::function<std::string(double)> format;
        std::function<bool(std::string_view, double&)> parse;
    };

    class CurveEdit : public Plot
    {
    public:
        // `xAxis`, `yAxis`: the ranges a point may take and the plot's (their labels name the units).
        CurveEdit(uint32_t id, std::string title, CurveSource& source, PlotAxis xAxis, PlotAxis yAxis);

        int selected() const noexcept { return selected_; }
        void select(int point) noexcept;
        const Curve16& shownCurve() const noexcept { return curve_; }
        // The point under p (within 9 px), -1: none.
        int pointAt(funkgui::Point p) const noexcept;

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point p) const override;
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices& h) override;
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController& g) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController& g) override;
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        const char* kind() const override { return "curve"; }

    private:
        void movePoint(int i, double x, double y);
        void refresh();

        CurveSource& source_;
        Curve16 curve_;
        int selected_ = -1, dragging_ = -1;
    };

    class MapEdit : public Widget
    {
    public:
        // x, y: the axes' text and ranges (lo, hi); z: the cells'. `labels`: the axes' names with their units.
        struct Axis
        {
            std::string label;
            double lo = 0.0, hi = 1.0;
            TableText text;
        };
        MapEdit(uint32_t id, std::string title, MapSource& source, Axis x, Axis y, Axis z);

        // The operating point (x, y in the axes' units), or false when there is none (the engine stopped).
        void setLive(std::function<bool(double& x, double& y)> live) { live_ = std::move(live); }

        // The selection: a cell (column, row), or the x axis's header (row -1) or the y axis's (column -1).
        int selectedColumn() const noexcept { return sx_; }
        int selectedRow() const noexcept { return sy_; }
        void select(int column, int row) noexcept;
        // The block of cells chosen: from the anchor (the cell chosen without Shift) to the selected cell, as the
        // first and last column and row. False (and nothing written) while an axis's header is chosen.
        bool block(int& column0, int& row0, int& column1, int& row1) const noexcept;
        // Choose a block: the anchor at (column0, row0), the selected cell at (column1, row1).
        void selectBlock(int column0, int row0, int column1, int row1) noexcept;
        bool editing() const noexcept { return editing_; }
        const Map16& shownMap() const noexcept { return map_; }
        // Where things are: a cell (column -1: the y header; row -1: the x header), the live cell (-1, -1: none).
        funkgui::Rect cellRect(int column, int row) const noexcept;
        bool liveCell(int& column, int& row) const noexcept;

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point p) const override;
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices& h) override;
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController& g) override;
        bool takesAllKeys() const override { return editing_; }
        void focusChanged(bool focused) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController& g) override;
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return liveShown_ == liveTarget_; }
        const char* kind() const override { return "map"; }

    private:
        bool hit(funkgui::Point p, int& column, int& row) const noexcept;
        double valueAt(int column, int row) const noexcept;
        void setValueAt(Map16& m, int column, int row, double v) const noexcept;
        const Axis& axisOf(int column, int row) const noexcept;
        bool commit();
        funkgui::Col heat(const Look& l, double z) const noexcept;

        MapSource& source_;
        Axis x_, y_, z_;
        Map16 map_;
        std::function<bool(double&, double&)> live_;
        int sx_ = 0, sy_ = 0, ax_ = 0, ay_ = 0;   // the selected cell, and the block's anchor
        bool editing_ = false, refused_ = false;
        funkgui::text::LineEdit edit_;
        bool dragging_ = false;
        float dragY0_ = 0.0f;
        double dragZ0_ = 0.0;
        // The live point as places across the table (column, row fractions), eased; NaN: none.
        std::array<float, 2> liveShown_{ -1.0f, -1.0f }, liveTarget_{ -1.0f, -1.0f };
    };
}
