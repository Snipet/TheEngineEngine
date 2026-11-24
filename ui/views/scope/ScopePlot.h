// ui/views/scope/ScopePlot.h: J2's Plot, its traces coloured by meaning (look::Palette, chosen at draw time so both
// themes read) and drawn solid or dashed, with marks of its own drawn over them and under the cursor (the valve events
// and the spark on a pressure trace, the firing orders on a spectrum, a legend), and a note shown in its area while it
// has no data ("not in this program yet"). SCOPE's plots and GARAGE's compare are ScopePlots. Canvas primitives only;
// opaque colours.
#pragma once

#include "app/Look.h"
#include "kit/widgets/Plot.h"

#include <funkgui/canvas/Canvas.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tee::ui
{
    // A polyline drawn as dashes `on` px long with `off` px gaps, measured along it (a dash runs on round a corner).
    // Points that are not finite break it.
    void dashedPolyline(funkgui::Canvas& c, const float* xs, const float* ys, int n, float width, funkgui::Col col,
                        float on = 7.0f, float off = 4.0f);

    // A legend's entry at (x, y top): a short sample of the line (solid or dashed, at its width) and its name in the
    // labels' capitals. Returns its width with the gap after it.
    float legendEntry(funkgui::Canvas& c, const look::Palette& p, float x, float y, std::string_view name, funkgui::Col line,
                      float width, bool dashed);

    class ScopePlot final : public Plot
    {
    public:
        // What a trace means: its ink in either theme.
        enum class Ink : std::uint8_t { value, text, label, dim, cold, hot, hotter, ok, warning, selected };
        struct Style
        {
            Ink ink = Ink::value;
            bool dashed = false;
            bool area = false;
            float width = look::kTrace;
        };
        using Overlay = std::function<void(funkgui::Canvas&, const look::Palette&, const ScopePlot&)>;

        ScopePlot(uint32_t id, std::string title) : Plot(id, std::move(title)) {}

        // The traces: series()[i] is drawn in styles()[i] (Plot's own colour and area are not used).
        std::vector<Style>& styles() noexcept { return styles_; }
        const std::vector<Style>& styles() const noexcept { return styles_; }
        // Replaces the traces: names, styles; their points are filled by the caller through series().
        void setTraces(std::vector<std::pair<std::string, Style>> traces);

        // Drawn after the traces, clipped to the plot's area (marks), and after that unclipped (a legend).
        void setOverlay(Overlay inside, Overlay outside = {})
        {
            inside_ = std::move(inside);
            outside_ = std::move(outside);
        }
        // A line of words centred in the area while no trace has a point: why it is empty.
        void setNote(std::string note);
        const std::string& note() const noexcept { return note_; }
        bool hasData() const noexcept;
        // A short text in the title line's right, while the cursor is not there (a peak, a level).
        void setCaption(std::string s);
        // The axes and the whole view at once; the view is kept (a person's zoom) unless the axes changed.
        void setAxes(double xlo, double xhi, bool xlog, double ylo, double yhi, bool ylog);

        static funkgui::Col inkOf(const look::Palette& p, Ink ink) noexcept;
        bool cursorOn() const noexcept { return cursorShown(); }
        funkgui::Point pointer() const noexcept { return pointer_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        uint64_t drawKey() const override;

    private:
        void drawTraces(funkgui::Canvas& c, const look::Palette& p) const;
        void drawReadout(funkgui::Canvas& c, const look::Palette& p) const;

        std::vector<Style> styles_;
        Overlay inside_, outside_;
        std::string note_, caption_;
    };
}
