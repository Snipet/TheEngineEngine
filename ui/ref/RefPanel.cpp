// ui/ref/RefPanel.cpp: see RefPanel.h.
#include "ref/RefPanel.h"

#include "kit/gui/GuiLayout.h"
#include "kit/gui/Style.h"
#include "kit/shaders/Registry.h"
#include "tee/ui/shaders/ref.h"

#include <funkgui/canvas/Canvas.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace tee::ui
{
    namespace
    {
        GuiPanel::Options guiOptions(const RefPanel::Options& o)
        {
            shaders::add(shaders::refShaders());         // the build's binaries for this panel, before it looks for them
            GuiPanel::Options g;
            g.width = o.width;
            g.height = o.height;
            g.minWidth = 480;
            g.minHeight = 300;
            g.name = "ref";
            g.scope = o.scope;
            g.shaders = o.shaders;
            return g;
        }

        bool hasGui(const Sources& files)
        {
            return std::any_of(files.begin(), files.end(), [](const SourceFile& f) {
                return f.name.size() > 4 && f.name.substr(f.name.size() - 4) == ".gui";
            });
        }
    }

    RefPanel::RefPanel(Sources files, Player& player, const AppServices& services, Options options)
        : GuiPanel(files, player, player, guiOptions(options)), player_(player), services_(services), hasFiles_(hasGui(files))
    {
        model_.send = services_.panelControls;
        gearHandle_ = player_.findValue(telemetry::kGear);
        stateHandle_ = player_.findValue(telemetry::kState);
        for (const GuiNode& n : spec().gui.nodes)
            hasKeyboard_ = hasKeyboard_ || n.kind == GuiKind::keyboard;

        using What = StripButton::What;
        start_ = addItem(std::make_unique<StripButton>(nextId(), What::start, model_));
        throttle_ = addItem(std::make_unique<StripThrottle>(nextId(), model_));
        down_ = addItem(std::make_unique<StripButton>(nextId(), What::shiftDown, model_));
        gear_ = addItem(std::make_unique<StripGear>(nextId(), model_));
        up_ = addItem(std::make_unique<StripButton>(nextId(), What::shiftUp, model_));
        if (hasKeyboard_)
            relayout();
        else
            setBottomReserve(static_cast<int>(kStripH + 2.0f * kStripPad));
    }

    // The strip: in the keyboard's cell on the page shown (hidden while that page is not), else under the interface.
    void RefPanel::placed()
    {
        if (start_ < 0)
            return;                                      // the base's constructor laying out: no strip yet
        bool shown = true;
        if (hasKeyboard_)
        {
            const std::vector<funkgui::Rect> keys = keyboardRects();
            shown = !keys.empty();
            strip_ = shown ? keys.front() : funkgui::Rect{};
        }
        else
            strip_ = { kStripPad, static_cast<float>(height() - bottomReserve()) + kStripPad,
                       static_cast<float>(width()) - 2.0f * kStripPad, kStripH };
        const float gap = 8.0f, buttonW = 72.0f, shiftW = 32.0f, gearW = 44.0f;
        const float h = strip_.h, y = strip_.y;
        float left = strip_.x;
        float right = strip_.right();
        const auto button = [&](int item) -> StripButton& {
            return static_cast<StripButton&>(*items()[static_cast<std::size_t>(item)]);
        };
        const float buttonTop = y + kGuiLabelH, buttonH = std::max(8.0f, h - kGuiLabelH);
        button(start_).place({ left, buttonTop, buttonW, buttonH }, shown);
        left += buttonW + gap * 2.0f;
        button(up_).place({ right - shiftW, buttonTop, shiftW, buttonH }, shown);
        right -= shiftW + gap * 0.5f;
        static_cast<StripGear&>(*items()[static_cast<std::size_t>(gear_)]).place({ right - gearW, y, gearW, h }, shown);
        right -= gearW + gap * 0.5f;
        button(down_).place({ right - shiftW, buttonTop, shiftW, buttonH }, shown);
        right -= shiftW + gap * 2.0f;
        static_cast<StripThrottle&>(*items()[static_cast<std::size_t>(throttle_)])
            .place({ left, y, std::max(40.0f, right - left), h }, shown && right - left >= 40.0f);
    }

    void RefPanel::tick(float dt)
    {
        const bool known = gearHandle_ >= 0;
        const int gear = known ? static_cast<int>(std::lround(player_.value(gearHandle_))) : 0;
        // The pedal the bar shows: the application's merged one when it says, else what the strip itself asks for.
        const float pedal = services_.controls ? std::clamp(services_.controls().throttle, 0.0f, 1.0f)
                                               : std::max(model_.dragging ? model_.drag : model_.rest, 0.0f);
        const int state = stateHandle_ >= 0 ? static_cast<int>(std::lround(player_.value(stateHandle_))) : 0;
        const bool running = state == 1 || state == 2;
        std::string error;
        if (services_.status)
        {
            const std::string status = services_.status();
            if (status.rfind("error:", 0) == 0)
                error = asciiText(status.substr(0, status.find('\n')));
        }
        if (known != model_.gearKnown || gear != model_.gear || pedal != model_.pedal || running != model_.running
            || error != error_)
        {
            model_.gearKnown = known;
            model_.gear = gear;
            model_.pedal = pedal;
            model_.running = running;
            error_ = std::move(error);
            changed();
        }
        GuiPanel::tick(dt);
    }

    void RefPanel::draw(funkgui::Canvas& c, const funkgui::Theme& theme)
    {
        GuiPanel::draw(c, theme);
        if (!error_.empty())
        {
            // Over the panel's top, in the theme's signal colour: one line, cut to the width.
            const style::Palette pal = style::palette(theme);
            const float w = static_cast<float>(width());
            c.rrect(0.0f, 0.0f, w, 24.0f, 0.0f, pal.code);
            c.text(fitText(c, error_, style::kSmallStrong, w - 20.0f).c_str(), 10.0f, c.capCentreTop(12.0f, style::kSmallStrong),
                   style::kSmallStrong, pal.danger);
        }
        if (!hasFiles_)
        {
            const style::Palette pal = style::palette(theme);
            c.text("The reference's panel is not in this build (sim/ref/panel.gui was not in the tree when it was made).",
                   16.0f, 32.0f, style::kBody, pal.soft);
        }
    }

    funkgui::Rect RefPanel::startRect() const { return static_cast<const StripButton&>(*items()[static_cast<std::size_t>(start_)]).rect(); }
    funkgui::Rect RefPanel::throttleRect() const
    {
        return static_cast<const StripThrottle&>(*items()[static_cast<std::size_t>(throttle_)]).track();
    }
    funkgui::Rect RefPanel::shiftRect(bool up) const
    {
        return static_cast<const StripButton&>(*items()[static_cast<std::size_t>(up ? up_ : down_)]).rect();
    }
    std::string RefPanel::gearText() const
    {
        return static_cast<const StripGear&>(*items()[static_cast<std::size_t>(gear_)]).text();
    }
}
