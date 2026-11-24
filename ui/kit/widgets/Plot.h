// ui/kit/widgets/Plot.h: data against an axis (card J2, item 6): the base of the dyno's curves, the scope, the cam
// editor and the compressor map of wave 2, and of CurveEdit here.
//
// Two axes, each linear or logarithmic, with ticks at round values and their numbers, a grid, and a label; several
// series, each a polyline (an opaque colour) and, if asked, the area under it (premixed over the ground); a cursor that
// follows the pointer with a readout of every series at its x; pan (drag) and zoom (the wheel: x about the pointer,
// Shift: y; a trackpad's pinch the same), a double-click back to the whole range. The view is the part of each axis's
// range shown; the axes' lo and hi are the whole of it.
#pragma once

#include "kit/widgets/Widget.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tee::ui
{
    struct PlotAxis
    {
        double lo = 0.0, hi = 1.0;           // the whole range (a log axis: both above 0)
        bool log = false;
        std::string label;                   // under the x axis, over the y axis ("RPM", "NM")
        std::function<std::string(double)> format;   // a tick's or the readout's number; none: by the step
    };

    struct PlotSeries
    {
        std::string name;
        std::vector<double> x, y;            // as many of each; a point that is not finite (or not > 0 on a log axis)
                                             // breaks the line
        std::optional<funkgui::Col> color;   // none: the n-th of the look's (the accent first)
        bool area = false;                   // the area under it, filled
        float width = 1.5f;
        bool points = false;                 // a disc at each point
    };

    class Plot : public Widget
    {
    public:
        Plot(uint32_t id, std::string title);

        PlotAxis& xAxis() noexcept { return x_; }
        PlotAxis& yAxis() noexcept { return y_; }
        const PlotAxis& xAxis() const noexcept { return x_; }
        const PlotAxis& yAxis() const noexcept { return y_; }
        std::vector<PlotSeries>& series() noexcept { return series_; }
        const std::vector<PlotSeries>& series() const noexcept { return series_; }
        // The data changed (a scope's new frame): drawKey moves.
        void touched() noexcept { ++revision_; }

        // The view (each axis's part shown), and back to the whole.
        void setView(double xlo, double xhi, double ylo, double yhi);
        void resetView();
        double viewX0() const noexcept { return vx0_; }
        double viewX1() const noexcept { return vx1_; }
        double viewY0() const noexcept { return vy0_; }
        double viewY1() const noexcept { return vy1_; }
        // Zoom by `factor` (> 1: in) about an x (or y) in data units.
        void zoomX(double factor, double about);
        void zoomY(double factor, double about);

        // Where the data is drawn, and the mapping both ways.
        funkgui::Rect plotArea() const noexcept;
        float pxX(double x) const noexcept;
        float pxY(double y) const noexcept;
        double dataX(float px) const noexcept;
        double dataY(float py) const noexcept;
        // A series' y at x, by straight lines between its points (NaN outside them).
        static double yAt(const PlotSeries& s, double x) noexcept;
        // The ticks the axis shows in its view across `px` px: values, and which are major (numbered).
        static void ticks(const PlotAxis& a, double v0, double v1, float px, std::vector<double>& out,
                          std::vector<bool>& major);

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point p) const override;
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices& h) override;
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        bool wheel(const funkgui::WheelEvent& e, funkgui::GestureController& g, double now) override;
        bool magnify(const funkgui::MagnifyEvent& e) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController& g) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController& g) override;
        uint64_t drawKey() const override;

    protected:
        const char* kind() const override { return "plot"; }
        // The pieces, for a subclass that draws more between them (CurveEdit).
        void drawFrame(funkgui::Canvas& c, const Look& l) const;
        void drawSeries(funkgui::Canvas& c, const Look& l) const;
        void drawCursor(funkgui::Canvas& c, const Look& l) const;
        funkgui::Col colorOf(const Look& l, std::size_t i) const noexcept;
        std::string numberOf(const PlotAxis& a, double v, double step) const;
        bool cursorShown() const noexcept;

        PlotAxis x_, y_;
        std::vector<PlotSeries> series_;
        double vx0_ = 0.0, vx1_ = 1.0, vy0_ = 0.0, vy1_ = 1.0;
        bool panning_ = false;
        funkgui::Point panFrom_{};
        double panX0_ = 0.0, panX1_ = 0.0, panY0_ = 0.0, panY1_ = 0.0;
        uint32_t revision_ = 0;
        mutable std::vector<float> xs_, ys_;              // draw()'s scratch: pixel places (not state)
    };
}
