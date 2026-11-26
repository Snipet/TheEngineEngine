// ui/kit/widgets/Tables.cpp: see Tables.h.
#include "kit/widgets/Tables.h"

#include "kit/gui/GuiLayout.h"

#include <funkgui/text/FontService.h>
#include <funkgui/text/TextFit.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;

        std::string plainNumber(double v)
        {
            char buf[32];
            std::snprintf(buf, sizeof buf, "%.3g", v);
            return buf;
        }

        // An axis's value as a place among its points: 0 at the first, n - 1 at the last, linear between.
        float placeOn(const std::array<double, kTableMax>& axis, int n, double v)
        {
            if (n <= 1)
                return 0.0f;
            if (v <= axis[0])
                return 0.0f;
            for (int i = 1; i < n; ++i)
                if (v <= axis[static_cast<std::size_t>(i)])
                {
                    const double a = axis[static_cast<std::size_t>(i - 1)], b = axis[static_cast<std::size_t>(i)];
                    return static_cast<float>(i - 1) + static_cast<float>(b > a ? (v - a) / (b - a) : 0.0);
                }
            return static_cast<float>(n - 1);
        }

        constexpr float kHeaderW = kGuiTableHeaderW, kHeaderH = kGuiTableHeaderH, kTitleH = kGuiTableTitleH;
        constexpr int kIds = 63;
    }

    // ---- CurveEdit ---------------------------------------------------------------------------------------------------
    CurveEdit::CurveEdit(uint32_t id, std::string title, CurveSource& source, PlotAxis xAxis, PlotAxis yAxis)
        : Plot(id, std::move(title)), source_(source)
    {
        x_ = std::move(xAxis);
        y_ = std::move(yAxis);
        resetView();
        series_.push_back({});
        series_[0].points = true;
        series_[0].width = 2.0f;
        refresh();
    }

    void CurveEdit::refresh()
    {
        curve_ = source_.curve();
        curve_.n = std::clamp(curve_.n, 0, kTableMax);
        PlotSeries& s = series_[0];
        s.x.assign(curve_.x.begin(), curve_.x.begin() + curve_.n);
        s.y.assign(curve_.y.begin(), curve_.y.begin() + curve_.n);
        if (selected_ >= curve_.n)
            selected_ = curve_.n - 1;
    }

    void CurveEdit::step(float) { refresh(); }

    void CurveEdit::select(int point) noexcept { selected_ = curve_.n > 0 ? std::clamp(point, 0, curve_.n - 1) : -1; }

    int CurveEdit::pointAt(funkgui::Point p) const noexcept
    {
        int best = -1;
        float bestD = 9.0f * 9.0f;
        for (int i = 0; i < curve_.n; ++i)
        {
            const float dx = pxX(curve_.x[static_cast<std::size_t>(i)]) - p.x, dy = pxY(curve_.y[static_cast<std::size_t>(i)]) - p.y;
            if (dx * dx + dy * dy <= bestD)
            {
                bestD = dx * dx + dy * dy;
                best = i;
            }
        }
        return best;
    }

    void CurveEdit::movePoint(int i, double x, double y)
    {
        Curve16 c = curve_;
        const auto k = static_cast<std::size_t>(i);
        const double lo = i > 0 ? c.x[k - 1] : x_.lo, hi = i + 1 < c.n ? c.x[k + 1] : x_.hi;
        c.x[k] = std::clamp(x, lo, hi);
        c.y[k] = std::clamp(y, y_.lo, y_.hi);
        if (c == curve_)
            return;
        source_.setCurve(c);
        refresh();
    }

    void CurveEdit::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        drawFrame(c, l);
        drawSeries(c, l);
        const int hot = dragging_ >= 0 ? dragging_ : (hover_ > 0.5f ? pointAt(pointer_) : -1);
        for (const int i : { hot, selected_ })
            if (i >= 0 && i < curve_.n)
                c.disc(pxX(curve_.x[static_cast<std::size_t>(i)]), pxY(curve_.y[static_cast<std::size_t>(i)]),
                       i == selected_ ? 5.5f : 4.5f, l.ground, 2.0f, i == selected_ ? l.accent : l.value);
        if (selected_ >= 0 && selected_ < curve_.n)
        {
            const auto k = static_cast<std::size_t>(selected_);
            const std::string s = "#" + std::to_string(selected_ + 1) + "  " + numberOf(x_, curve_.x[k], (vx1_ - vx0_) / 100.0)
                                + (x_.label.empty() ? "" : " " + x_.label) + "  " + numberOf(y_, curve_.y[k], (vy1_ - vy0_) / 100.0)
                                + (y_.label.empty() ? "" : " " + y_.label);
            c.text(s.c_str(), plotArea().right(), bounds_.y + 3.0f, kWidgetSmall, l.value, funkgui::Align::right);
        }
        else
            drawCursor(c, l);
        if (focusRing)
            ring(c, l, plotArea(), 0.0f);
    }

    funkgui::Cursor CurveEdit::cursorAt(funkgui::Point p) const
    {
        if (dragging_ >= 0)
            return funkgui::Cursor::grabbing;
        return pointAt(p) >= 0 ? funkgui::Cursor::grab : Plot::cursorAt(p);
    }

    void CurveEdit::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices& h)
    {
        const int i = e.popup ? -1 : pointAt({ e.x, e.y });
        if (i < 0)
        {
            Plot::pointerDown(e, g, h);
            return;
        }
        selected_ = dragging_ = i;
        source_.beginEdit();
    }

    void CurveEdit::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController& g)
    {
        if (dragging_ < 0)
        {
            Plot::pointerDrag(e, g);
            return;
        }
        const Rect a = plotArea();
        movePoint(dragging_, dataX(std::clamp(e.x, a.x, a.right())), dataY(std::clamp(e.y, a.y, a.bottom())));
    }

    void CurveEdit::pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController& g)
    {
        if (dragging_ < 0)
        {
            Plot::pointerUp(e, g);
            return;
        }
        dragging_ = -1;
        source_.endEdit();
    }

    void CurveEdit::doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController& g)
    {
        if (pointAt({ e.x, e.y }) < 0)
            Plot::doubleClick(e, g);
    }

    bool CurveEdit::key(const funkgui::KeyEvent& e, funkgui::GestureController& g)
    {
        if (curve_.n == 0)
            return Plot::key(e, g);
        if (e.key == funkgui::Key::left || e.key == funkgui::Key::right)
        {
            select(selected_ < 0 ? 0 : selected_ + (e.key == funkgui::Key::left ? -1 : 1));
            return true;
        }
        if ((e.key == funkgui::Key::up || e.key == funkgui::Key::down) && selected_ >= 0)
        {
            const double stepY = (y_.hi - y_.lo) * (e.mods.shift ? 0.001 : 0.01) * (e.key == funkgui::Key::up ? 1.0 : -1.0);
            const auto k = static_cast<std::size_t>(selected_);
            source_.beginEdit();
            movePoint(selected_, curve_.x[k], curve_.y[k] + stepY);
            source_.endEdit();
            return true;
        }
        if (e.key == funkgui::Key::escape && selected_ >= 0)
        {
            selected_ = -1;
            return true;
        }
        return Plot::key(e, g);
    }

    void CurveEdit::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem group;
        group.id = id_;
        group.role = funkgui::a11yrole::group;
        group.bounds = bounds_;
        group.title = title_;
        group.description = std::to_string(curve_.n) + " points";
        out.push_back(group);
        for (int i = 0; i < curve_.n && i < kIds; ++i)
        {
            const auto k = static_cast<std::size_t>(i);
            funkgui::A11yItem it;
            it.id = id_ + 1 + static_cast<uint32_t>(i);
            it.parent = id_;
            it.role = funkgui::A11yRole::slider;
            const float px = pxX(curve_.x[k]), py = pxY(curve_.y[k]);
            it.bounds = { px - 5.0f, py - 5.0f, 10.0f, 10.0f };
            it.title = title_ + " point " + std::to_string(i + 1) + " at " + numberOf(x_, curve_.x[k], 1.0);
            it.value = numberOf(y_, curve_.y[k], (y_.hi - y_.lo) / 100.0);
            it.v = curve_.y[k];
            it.lo = y_.lo;
            it.hi = y_.hi;
            it.step = (y_.hi - y_.lo) / 100.0;
            out.push_back(std::move(it));
        }
    }

    bool CurveEdit::a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController&)
    {
        if (id <= id_ || id > id_ + static_cast<uint32_t>(curve_.n))
            return false;
        const int i = static_cast<int>(id - id_ - 1);
        const auto k = static_cast<std::size_t>(i);
        const double step = (y_.hi - y_.lo) / 100.0;
        double y = curve_.y[k];
        if (a == funkgui::A11yAction::increment)
            y += step;
        else if (a == funkgui::A11yAction::decrement)
            y -= step;
        else if (a == funkgui::A11yAction::setValue)
            y = v;
        else
            return false;
        source_.beginEdit();
        movePoint(i, curve_.x[k], y);
        source_.endEdit();
        return true;
    }

    uint64_t CurveEdit::drawKey() const
    {
        DrawKey k;
        const Curve16 c = source_.curve();
        for (int i = 0; i < c.n && i < kTableMax; ++i)
            k.mix(static_cast<float>(c.x[static_cast<std::size_t>(i)])).mix(static_cast<float>(c.y[static_cast<std::size_t>(i)]));
        return k.mix(selected_).mix(Plot::drawKey()).h;
    }

    // ---- MapEdit -----------------------------------------------------------------------------------------------------
    MapEdit::MapEdit(uint32_t id, std::string title, MapSource& source, Axis x, Axis y, Axis z)
        : Widget(id), source_(source), x_(std::move(x)), y_(std::move(y)), z_(std::move(z)), map_(source.map())
    {
        title_ = std::move(title);
    }

    void MapEdit::step(float dt)
    {
        if (burst_ && (burstRest_ += std::max(dt, 0.0f)) >= 0.5f)
        {
            burst_ = false;                  // the controller's burst rested: one undo step
            source_.endEdit();
        }
        map_ = source_.map();
        map_.nx = std::clamp(map_.nx, 0, kTableMax);
        map_.ny = std::clamp(map_.ny, 0, kTableMax);
        sx_ = std::clamp(sx_, -1, std::max(0, map_.nx - 1));
        sy_ = std::clamp(sy_, -1, std::max(0, map_.ny - 1));
        double lx = 0.0, ly = 0.0;
        if (live_ && map_.nx > 0 && map_.ny > 0 && live_(lx, ly))
            liveTarget_ = { placeOn(map_.x, map_.nx, lx), placeOn(map_.y, map_.ny, ly) };
        else
            liveTarget_ = { -1.0f, -1.0f };
        for (std::size_t k = 0; k < 2; ++k)
            liveShown_[k] = liveTarget_[k] < 0.0f || liveShown_[k] < 0.0f
                                ? liveTarget_[k]
                                : funkgui::ease::toward(liveShown_[k], liveTarget_[k], dt, 0.05f, 1e-3f);
    }

    int MapEdit::lines() const noexcept
    {
        if (map_.ny != 1 || map_.nx <= 1)
            return 1;
        const int room = std::max(1, static_cast<int>((bounds_.h - kTitleH) / kGuiTableLineH));
        return std::clamp(guiTableLines(map_.nx, bounds_.w), 1, room);
    }

    Rect MapEdit::cellRect(int column, int row) const noexcept
    {
        const float top = bounds_.y + kTitleH;
        if (const int n = lines(); n > 1)
        {
            // One row on n lines: each line its numbers' header and its cells, across the whole width.
            if (column < 0 || column >= map_.nx)
                return {};
            const int perLine = (map_.nx + n - 1) / n;
            const float cw = bounds_.w / static_cast<float>(perLine), lineH = (bounds_.h - kTitleH) / static_cast<float>(n);
            const float x = bounds_.x + cw * static_cast<float>(column % perLine), y = top + lineH * static_cast<float>(column / perLine);
            return row < 0 ? Rect{ x, y, cw, kHeaderH } : Rect{ x, y + kHeaderH, cw, lineH - kHeaderH };
        }
        const float cw = map_.nx > 0 ? (bounds_.w - kHeaderW) / static_cast<float>(map_.nx) : 0.0f;
        const float ch = map_.ny > 0 ? (bounds_.h - kTitleH - kHeaderH) / static_cast<float>(map_.ny) : 0.0f;
        const float x = column < 0 ? bounds_.x : bounds_.x + kHeaderW + cw * static_cast<float>(column);
        const float y = row < 0 ? top : top + kHeaderH + ch * static_cast<float>(row);
        return { x, y, column < 0 ? kHeaderW : cw, row < 0 ? kHeaderH : ch };
    }

    bool MapEdit::hit(funkgui::Point p, int& column, int& row) const noexcept
    {
        if (map_.nx <= 0 || map_.ny <= 0)
            return false;
        const Rect first = cellRect(0, 0);
        const float top = bounds_.y + kTitleH;
        if (p.y < top || p.x < bounds_.x || !bounds_.contains(p))
            return false;
        if (const int n = lines(); n > 1)
        {
            const int perLine = (map_.nx + n - 1) / n;
            const float lineH = (bounds_.h - kTitleH) / static_cast<float>(n);
            const int line = std::min(n - 1, static_cast<int>((p.y - top) / lineH));
            const int i = line * perLine + std::min(perLine - 1, static_cast<int>((p.x - bounds_.x) / first.w));
            if (i >= map_.nx)
                return false;                            // past the last cell of the last line
            column = i;
            row = p.y - top - lineH * static_cast<float>(line) < kHeaderH ? -1 : 0;
            return true;
        }
        column = p.x < first.x ? -1 : std::min(map_.nx - 1, static_cast<int>((p.x - first.x) / first.w));
        row = p.y < first.y ? -1 : std::min(map_.ny - 1, static_cast<int>((p.y - first.y) / first.h));
        return !(column < 0 && row < 0);                 // the corner is nothing
    }

    bool MapEdit::liveCell(int& column, int& row) const noexcept
    {
        if (liveTarget_[0] < 0.0f)
            return false;
        column = static_cast<int>(std::lround(liveTarget_[0]));
        row = static_cast<int>(std::lround(liveTarget_[1]));
        return true;
    }

    double MapEdit::valueAt(int column, int row) const noexcept
    {
        if (row < 0)
            return map_.x[static_cast<std::size_t>(column)];
        if (column < 0)
            return map_.y[static_cast<std::size_t>(row)];
        return map_.z[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
    }

    // A cell's value, or an axis's point kept between its neighbours (an axis rises).
    void MapEdit::setValueAt(Map16& m, int column, int row, double v) const noexcept
    {
        if (row < 0 || column < 0)
        {
            auto& axis = row < 0 ? m.x : m.y;
            const int i = row < 0 ? column : row, n = row < 0 ? m.nx : m.ny;
            const Axis& a = axisOf(column, row);
            const double lo = i > 0 ? axis[static_cast<std::size_t>(i - 1)] : a.lo;
            const double hi = i + 1 < n ? axis[static_cast<std::size_t>(i + 1)] : a.hi;
            axis[static_cast<std::size_t>(i)] = std::clamp(v, lo, hi);
            return;
        }
        m.z[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = std::clamp(v, z_.lo, z_.hi);
    }

    const MapEdit::Axis& MapEdit::axisOf(int column, int row) const noexcept { return row < 0 ? x_ : (column < 0 ? y_ : z_); }

    void MapEdit::select(int column, int row) noexcept
    {
        if (editing_)
            commit();
        sx_ = std::clamp(column, lines() > 1 ? 0 : -1, std::max(0, map_.nx - 1));   // wrapped: no y header
        sy_ = std::clamp(row, -1, std::max(0, map_.ny - 1));
        if (sx_ < 0 && sy_ < 0)
            sy_ = 0;
        ax_ = sx_;
        ay_ = sy_;
    }

    bool MapEdit::block(int& column0, int& row0, int& column1, int& row1) const noexcept
    {
        if (sx_ < 0 || sy_ < 0 || map_.nx <= 0 || map_.ny <= 0)
            return false;
        const int ax = std::clamp(ax_, 0, map_.nx - 1), ay = std::clamp(ay_, 0, map_.ny - 1);
        column0 = std::min(ax, sx_);
        column1 = std::max(ax, sx_);
        row0 = std::min(ay, sy_);
        row1 = std::max(ay, sy_);
        return true;
    }

    void MapEdit::selectBlock(int column0, int row0, int column1, int row1) noexcept
    {
        select(column0, row0);
        if (sx_ < 0 || sy_ < 0)
            return;
        sx_ = std::clamp(column1, 0, std::max(0, map_.nx - 1));
        sy_ = std::clamp(row1, 0, std::max(0, map_.ny - 1));
    }

    Col MapEdit::heat(const Look& l, double z) const noexcept
    {
        const float t = z_.hi > z_.lo ? static_cast<float>(std::clamp((z - z_.lo) / (z_.hi - z_.lo), 0.0, 1.0)) : 0.0f;
        return t < 0.5f ? funkgui::mix(l.face, l.accentDim, t * 2.0f) : funkgui::mix(l.accentDim, l.accent, (t - 0.5f) * 2.0f);
    }

    void MapEdit::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        const auto fmt = [](const Axis& a, double v) { return a.text.format ? a.text.format(v) : plainNumber(v); };
        // The title, and what the axes are in the room beside it (none when too little: a narrow column).
        // While a refused typing is kept: the refusal's sentence on that line instead (the error colour), the whole width.
        const std::string title = upper(title_);
        const bool saying = refused_ && !refusal_.empty();
        if (saying)
            c.text(fitText(c, refusal_, kWidgetSmall, bounds_.w).c_str(), bounds_.x, bounds_.y + 3.0f, kWidgetSmall, l.error);
        else
            c.text(title.c_str(), bounds_.x, bounds_.y + 3.0f, kWidgetLabel, l.label);
        const float axesRoom = bounds_.w - c.textWidth(title.c_str(), kWidgetLabel) - 16.0f;
        if (axesRoom >= 60.0f && !saying)
        {
            const std::string axes = upper(z_.label) + " BY " + upper(x_.label) + " ACROSS, " + upper(y_.label) + " DOWN";
            c.text(fitText(c, axes, kWidgetSmall, axesRoom).c_str(), bounds_.right(), bounds_.y + 3.0f, kWidgetSmall, l.dim,
                   funkgui::Align::right);
        }
        if (map_.nx <= 0 || map_.ny <= 0)
            return;
        const Canvas::ClipScope clip(c, bounds_);
        const funkgui::TextStyle cellText = kWidgetSmall;
        const bool wrapped = lines() > 1;
        int liveX = -1, liveY = -1;
        const bool live = liveCell(liveX, liveY);
        // The headers; text cut to its cell (a choice's name can be longer than a cell is wide).
        for (int i = 0; i < map_.nx; ++i)
        {
            const Rect r = cellRect(i, -1);
            const bool sel = sy_ < 0 && sx_ == i, lit = live && liveX == i;
            c.rrect(r.x + 0.5f, r.y + 0.5f, r.w - 1.0f, r.h - 1.0f, 2.0f, sel ? l.accentDim : l.faceHi);
            c.text(fitText(c, fmt(x_, map_.x[static_cast<std::size_t>(i)]), cellText, r.w - 4.0f).c_str(), r.centreX(),
                   c.capCentreTop(r.centreY(), cellText), cellText, lit ? l.accent : l.text, funkgui::Align::centre);
        }
        for (int j = 0; j < map_.ny && !wrapped; ++j)
        {
            const Rect r = cellRect(-1, j);
            const bool sel = sx_ < 0 && sy_ == j, lit = live && liveY == j;
            c.rrect(r.x + 0.5f, r.y + 0.5f, r.w - 1.0f, r.h - 1.0f, 2.0f, sel ? l.accentDim : l.faceHi);
            c.text(fitText(c, fmt(y_, map_.y[static_cast<std::size_t>(j)]), cellText, r.w - 8.0f).c_str(), r.right() - 5.0f,
                   c.capCentreTop(r.centreY(), cellText), cellText, lit ? l.accent : l.text, funkgui::Align::right);
        }
        // The cells as a heat map, with their numbers when there is room.
        const Rect first = cellRect(0, 0);
        const bool numbers = first.w >= 28.0f && first.h >= 12.0f;
        for (int j = 0; j < map_.ny; ++j)
            for (int i = 0; i < map_.nx; ++i)
            {
                const Rect r = cellRect(i, j);
                const double z = map_.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)];
                const Col fill = heat(l, z);
                c.rrect(r.x + 0.5f, r.y + 0.5f, r.w - 1.0f, r.h - 1.0f, 1.5f, fill);
                if (numbers)
                {
                    const bool bright = fill.r + fill.g + fill.b > 3 * 150;
                    c.text(fitText(c, fmt(z_, z), cellText, r.w - 4.0f).c_str(), r.centreX(), c.capCentreTop(r.centreY(), cellText),
                           cellText, bright ? l.ground : l.value, funkgui::Align::centre);
                }
            }
        // The live cell, and the operating point between the cells (wrapped: at the live cell's centre, the places
        // between cells being on other lines).
        if (live)
        {
            const Rect r = cellRect(liveX, liveY);
            c.rrect(r.x, r.y, r.w, r.h, 2.0f, kClear, 2.0f, l.value);
            const float px = wrapped ? r.centreX() : first.x + (liveShown_[0] + 0.5f) * first.w;
            const float py = wrapped ? r.centreY() : first.y + (liveShown_[1] + 0.5f) * first.h;
            c.disc(px, py, 3.5f, l.value, 1.5f, l.ground);
        }
        // The block, when it is more than one cell: its outline (wrapped: each cell's, the block crossing lines).
        int b0 = 0, r0 = 0, b1 = 0, r1 = 0;
        if (block(b0, r0, b1, r1) && (b1 > b0 || r1 > r0))
        {
            if (wrapped)
                for (int i = b0; i <= b1; ++i)
                {
                    const Rect a = cellRect(i, 0);
                    c.rrect(a.x, a.y, a.w, a.h, 2.0f, kClear, 2.0f, l.accent);
                }
            else
            {
                const Rect a = cellRect(b0, r0), z = cellRect(b1, r1);
                c.rrect(a.x - 1.0f, a.y - 1.0f, z.right() - a.x + 2.0f, z.bottom() - a.y + 2.0f, 3.0f, kClear, 2.0f, l.accent);
            }
        }
        // The selection, and the value being typed in it.
        const Rect s = cellRect(sx_, sy_);
        c.rrect(s.x, s.y, s.w, s.h, 2.0f, kClear, focusRing || editing_ ? 2.0f : 1.5f, l.accent);
        funkgui::Rect caret, typed;
        if (caretRect(caret, typed))
        {
            c.rrect(s.x + 1.0f, s.y + 1.0f, s.w - 2.0f, s.h - 2.0f, 2.0f, l.ground);
            c.text(edit_.buffer.c_str(), typed.x, c.capCentreTop(s.centreY(), cellText), cellText, refused_ ? l.error : l.value);
            c.segment(caret.centreX(), caret.y, caret.centreX(), caret.bottom(), caret.w, l.accent);
        }
    }

    bool MapEdit::focusFirstCell() noexcept
    {
        select(0, 0);
        return map_.nx > 0 && map_.ny > 0;
    }

    bool MapEdit::nudgeCell(int dir, bool fine)
    {
        if (map_.nx <= 0 || map_.ny <= 0 || (sx_ < 0 && sy_ < 0))
            return false;
        const Axis& a = axisOf(sx_, sy_);
        const auto fmt = [&a](double v) { return a.text.format ? a.text.format(v) : plainNumber(v); };
        const double now = valueAt(sx_, sy_);
        double probe = 0.0;
        if (!a.text.parse || !a.text.parse(fmt(now), probe))
            return false;                    // not editable (an array's numbers)
        const double unit = a.step > 0.0 ? a.step : (a.hi - a.lo) / 100.0;
        double v = std::clamp(now + static_cast<double>(dir) * unit * (fine && a.step <= 0.0 ? 0.1 : 1.0), std::min(a.lo, a.hi),
                              std::max(a.lo, a.hi));
        if (v == now)
            return false;
        const std::string t = fmt(v);
        double taken = 0.0;
        if (!a.text.parse(t, taken))         // refused (FieldDoc::accepts through the binding): said, nothing moved
        {
            refused_ = true;
            refusal_ = a.text.refusal ? a.text.refusal(t) : std::string();
            return false;
        }
        if (a.step <= 0.0)
            taken = v;                       // the nudge's value, not its text's rounding
        if (!burst_)
        {
            burst_ = true;
            source_.beginEdit();
        }
        burstRest_ = 0.0f;
        Map16 m = map_;
        setValueAt(m, sx_, sy_, taken);
        if (m == map_)
            return false;
        source_.setMap(m);
        map_ = m;
        refused_ = false;
        refusal_.clear();
        return true;
    }

    bool MapEdit::edit(EditCommand cmd, bool fine)
    {
        using C = EditCommand;
        if (editing_)
        {
            if (cmd == C::back)
            {
                editing_ = refused_ = false;
                refusal_.clear();
                return true;
            }
            if (!commit())
                return true;                 // refused: the typing stays, its sentence said
        }
        const int px = sx_, py = sy_;
        switch (cmd)
        {
            case C::rowUp:
            case C::rowDown:
            {
                const int d = cmd == C::rowUp ? -1 : 1;
                if (lines() > 1)             // an array on lines: a line up or down
                {
                    const int per = (map_.nx + lines() - 1) / lines();
                    if (sx_ + d * per < 0 || sx_ + d * per >= map_.nx)
                        return false;
                    select(sx_ + d * per, sy_);
                }
                else
                    select(sx_, sy_ + d);
                return sx_ != px || sy_ != py;
            }
            case C::pagePrev:
            case C::pageNext:
                select(sx_ + (cmd == C::pagePrev ? -1 : 1), sy_);
                return sx_ != px || sy_ != py;
            case C::valueDown:
            case C::valueUp:
            case C::valueDownCoarse:
            case C::valueUpCoarse:
            {
                const int dir = cmd == C::valueUp || cmd == C::valueUpCoarse ? 1 : -1;
                const int n = cmd == C::valueDownCoarse || cmd == C::valueUpCoarse ? 10 : 1;
                bool moved = false;
                for (int k = 0; k < n; ++k)
                    if (nudgeCell(dir, fine))
                        moved = true;
                    else
                        break;
                return moved || (refused_ && !refusal_.empty());
            }
            default: return false;           // activate (no typing from a controller), back, the rest: the frame's
        }
    }

    bool MapEdit::caretRect(funkgui::Rect& caret, funkgui::Rect& text) const noexcept
    {
        if (!editing_)
            return false;
        const funkgui::TextStyle st = kWidgetSmall;
        const funkgui::FontAtlasSdf& atlas = funkgui::FontService::get().atlas();
        const std::string& b = edit_.buffer;
        const std::size_t at = static_cast<std::size_t>(std::clamp<int>(edit_.caret, 0, static_cast<int>(b.size())));
        constexpr float kCaretW = 1.0f, kGap = 1.5f;   // the gap: more than the caret's width
        const Rect s = cellRect(sx_, sy_);
        const float w = funkgui::text::width(atlas, b.c_str(), st);
        const float lead = at == b.size() ? kGap + kCaretW : 0.0f;   // the caret after the text needs its room
        const float tx = s.centreX() - (w + lead) * 0.5f;
        const float lineH = std::min(st.px, s.h - 4.0f);             // the text's line, never past the cell
        text = { tx, s.centreY() - lineH * 0.5f, w, lineH };
        float cx = tx + funkgui::text::width(atlas, b.substr(0, at).c_str(), st);
        if (at == b.size())
            cx += kGap;
        caret = { cx, s.centreY() - lineH * 0.5f, kCaretW, lineH };
        return true;
    }

    funkgui::Cursor MapEdit::cursorAt(funkgui::Point p) const
    {
        int i = 0, j = 0;
        if (dragging_)
            return funkgui::Cursor::upDown;
        return hit(p, i, j) ? (i >= 0 && j >= 0 ? funkgui::Cursor::upDown : funkgui::Cursor::pointingHand)
                            : funkgui::Cursor::normal;
    }

    void MapEdit::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        int i = 0, j = 0;
        if (e.popup || !hit({ e.x, e.y }, i, j))
            return;
        if (e.mods.shift && i >= 0 && j >= 0 && sx_ >= 0 && sy_ >= 0)
        {
            if (editing_)
                commit();
            sx_ = i;                                     // the block grows from its anchor to here
            sy_ = j;
            return;
        }
        select(i, j);
        if (i >= 0 && j >= 0)                            // a cell: a drag up raises it
        {
            dragging_ = true;
            dragY0_ = e.y;
            dragZ0_ = valueAt(i, j);
            source_.beginEdit();
        }
    }

    void MapEdit::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (!dragging_)
            return;
        const double per = (z_.hi - z_.lo) / (e.mods.shift ? 2000.0 : 200.0);   // 200 px the whole range
        Map16 m = map_;
        setValueAt(m, sx_, sy_, dragZ0_ + static_cast<double>(dragY0_ - e.y) * per);
        if (!(m == map_))
        {
            source_.setMap(m);
            map_ = m;
        }
    }

    void MapEdit::pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&)
    {
        if (std::exchange(dragging_, false))
            source_.endEdit();
    }

    bool MapEdit::commit()
    {
        if (!editing_)
            return true;
        const Axis& a = axisOf(sx_, sy_);
        double v = 0.0;
        const bool ok = a.text.parse ? a.text.parse(edit_.buffer, v) : false;
        if (!ok)
        {
            refused_ = true;
            refusal_ = a.text.refusal ? a.text.refusal(edit_.buffer) : std::string();
            return false;
        }
        Map16 m = map_;
        setValueAt(m, sx_, sy_, v);
        source_.beginEdit();
        source_.setMap(m);
        source_.endEdit();
        map_ = m;
        editing_ = refused_ = false;
        refusal_.clear();
        return true;
    }

    bool MapEdit::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        using funkgui::Key;
        if (editing_)
        {
            if (e.key == Key::enter)
            {
                if (commit() && sy_ + 1 < map_.ny)
                    ++sy_;                               // as a spreadsheet: Enter keeps it and goes down a row
                return true;
            }
            if (e.key == Key::escape)
            {
                editing_ = refused_ = false;
                refusal_.clear();
                return true;
            }
            if (e.key == Key::tab)
            {
                if (!commit())
                    return true;
                select(sx_ + (e.mods.shift ? -1 : 1), sy_);
                return true;
            }
            refused_ = false;              // a key after a refusal: the sentence goes, the typing is a new try
            refusal_.clear();
            edit_.key(e, 16);
            return true;
        }
        switch (e.key)
        {
            case Key::left:
            case Key::right:
            case Key::up:
            case Key::down:
            {
                const int dx = e.key == Key::left ? -1 : e.key == Key::right ? 1 : 0;
                const int dy = e.key == Key::up ? -1 : e.key == Key::down ? 1 : 0;
                if (e.mods.shift && sx_ >= 0 && sy_ >= 0)
                {
                    sx_ = std::clamp(sx_ + dx, 0, std::max(0, map_.nx - 1));   // the block grows or shrinks
                    sy_ = std::clamp(sy_ + dy, 0, std::max(0, map_.ny - 1));
                }
                else
                    select(sx_ + dx, sy_ + dy);
                return true;
            }
            case Key::enter:
                edit_.set(axisOf(sx_, sy_).text.format ? axisOf(sx_, sy_).text.format(valueAt(sx_, sy_)) : plainNumber(valueAt(sx_, sy_)), 16);
                editing_ = true;
                return true;
            case Key::character:
                if ((e.ch >= '0' && e.ch <= '9') || e.ch == '-' || e.ch == '.')
                {
                    edit_.set("", 16);
                    edit_.key(e, 16);
                    editing_ = true;
                    refused_ = false;
                    refusal_.clear();
                    return true;
                }
                return false;
            default: return false;
        }
    }

    void MapEdit::focusChanged(bool focused)
    {
        if (!focused && editing_ && !commit())
        {
            editing_ = refused_ = false;
            refusal_.clear();
        }
    }

    void MapEdit::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem table;
        table.id = id_;
        table.role = funkgui::a11yrole::table;
        table.bounds = bounds_;
        table.title = title_;
        table.rows = map_.ny + 1;
        table.cols = map_.nx + 1;
        out.push_back(table);
        if (map_.nx <= 0 || map_.ny <= 0)
            return;
        funkgui::A11yItem row;
        row.id = id_ + 1;
        row.parent = id_;
        row.role = funkgui::a11yrole::row;
        row.row = sy_ + 1;
        row.bounds = cellRect(-1, sy_);
        out.push_back(row);
        funkgui::A11yItem cell;
        cell.id = id_ + 2;
        cell.parent = id_ + 1;
        cell.role = funkgui::A11yRole::slider;
        cell.col = sx_ + 1;
        cell.bounds = cellRect(sx_, sy_);
        const Axis& a = axisOf(sx_, sy_);
        const double v = valueAt(sx_, sy_);
        cell.title = sy_ < 0 ? x_.label + " " + std::to_string(sx_ + 1)
                   : sx_ < 0 ? y_.label + " " + std::to_string(sy_ + 1)
                             : z_.label + " at " + (x_.text.format ? x_.text.format(map_.x[static_cast<std::size_t>(sx_)]) : "")
                                   + ", " + (y_.text.format ? y_.text.format(map_.y[static_cast<std::size_t>(sy_)]) : "");
        cell.value = a.text.format ? a.text.format(v) : plainNumber(v);
        cell.v = v;
        cell.lo = a.lo;
        cell.hi = a.hi;
        cell.step = (a.hi - a.lo) / 100.0;
        out.push_back(cell);
    }

    bool MapEdit::a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController&)
    {
        if (id != id_ + 2)
            return false;
        const Axis& ax = axisOf(sx_, sy_);
        double now = valueAt(sx_, sy_);
        if (a == funkgui::A11yAction::increment)
            now += (ax.hi - ax.lo) / 100.0;
        else if (a == funkgui::A11yAction::decrement)
            now -= (ax.hi - ax.lo) / 100.0;
        else if (a == funkgui::A11yAction::setValue)
            now = v;
        else
            return false;
        Map16 m = map_;
        setValueAt(m, sx_, sy_, now);
        source_.beginEdit();
        source_.setMap(m);
        source_.endEdit();
        map_ = m;
        return true;
    }

    uint64_t MapEdit::drawKey() const
    {
        DrawKey k;
        const Map16 m = source_.map();
        for (int j = 0; j < m.ny && j < kTableMax; ++j)
            for (int i = 0; i < m.nx && i < kTableMax; ++i)
                k.mix(static_cast<float>(m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)]));
        for (int i = 0; i < kTableMax; ++i)
            k.mix(static_cast<float>(m.x[static_cast<std::size_t>(i)])).mix(static_cast<float>(m.y[static_cast<std::size_t>(i)]));
        return k.mix(sx_).mix(sy_).mix(ax_).mix(ay_).mix(editing_).mix(refused_).mix(refusal_).mix(edit_.buffer).mix(liveTarget_[0]).mix(liveTarget_[1]).h;
    }
}
