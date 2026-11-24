// shell/panel/PlaceholderPanel.cpp: the shell's own plain Panel (PlaceholderPanel.h). Plain and correct: the engine's speed, gear and state as numbers, a start button, a throttle strip, FunkGui's Dial for every
// parameter in a grid, and a footer that says what to press or, when the program did not compile, why, in the
// compiler's words.
//
// It follows a Panel's rules (funkgui/panel/Panel.h): time only from tick(dt), draw() reads state and never advances
// it, every write to a parameter through the GestureController.
#include "panel/PlaceholderPanel.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Col.h>
#include <funkgui/core/Geometry.h>
#include <funkgui/core/Theme.h>
#include <funkgui/core/TypeScale.h>
#include <funkgui/panel/HostServices.h>
#include <funkgui/params/GestureController.h>
#include <funkgui/params/ParamPort.h>
#include <funkgui/widgets/Dial.h>
#include <funkgui/widgets/ValueModel.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Point;
        using funkgui::Rect;
        using funkgui::Theme;
        namespace type = funkgui::type;

        constexpr std::size_t kLabelChars = 8;      // what a dial's cell holds of a label

        // ---- a parameter as FunkGui's widgets see it ---------------------------------------------------------------

        bool discrete(const ParamInfo& p) noexcept
        {
            return p.kind != ParamInfo::Kind::real;
        }

        int stepsOf(const ParamInfo& p) noexcept
        {
            return static_cast<int>(std::lround(p.max - p.min)) + 1;
        }

        // The control's travel, 0 to 1, for a value, and back (a logarithmic taper where the parameter asks).
        float toHost01(const ParamInfo& p, double v) noexcept
        {
            if (!(p.max > p.min))
                return 0.0f;
            double t = 0.0;
            if (p.logTaper && p.min > 0.0)
                t = std::log(std::clamp(v, p.min, p.max) / p.min) / std::log(p.max / p.min);
            else
                t = (v - p.min) / (p.max - p.min);
            return static_cast<float>(std::clamp(t, 0.0, 1.0));
        }

        double fromHost01(const ParamInfo& p, float h) noexcept
        {
            const double t = std::clamp(static_cast<double>(h), 0.0, 1.0);
            double v = 0.0;
            if (p.logTaper && p.min > 0.0)
                v = p.min * std::pow(p.max / p.min, t);
            else
                v = p.min + t * (p.max - p.min);
            return discrete(p) ? std::round(v) : v;
        }

        class Port final : public funkgui::ParamPort
        {
        public:
            Port(ParamSource& source, int index) : source_(source), index_(index), info_(source.info(index)) {}

            float value01() const override { return toHost01(info_, source_.get(index_)); }
            float default01() const override { return toHost01(info_, info_.def); }
            int numSteps() const override { return discrete(info_) ? stepsOf(info_) : 0x7fffffff; }
            void beginGesture() override {}
            void setValue01(float v) override { source_.set(index_, fromHost01(info_, v)); }
            void endGesture() override {}
            const char* id() const override { return info_.id.c_str(); }
            void* native() const override { return nullptr; }

            const ParamInfo& info() const noexcept { return info_; }
            double plain() const { return source_.get(index_); }

        private:
            ParamSource& source_;
            int index_;
            ParamInfo info_;                     // a copy: the texts a ValueView points at live here
        };

        std::string upper(std::string s)
        {
            for (char& c : s)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return s;
        }

        class Model final : public funkgui::ValueModel
        {
        public:
            explicit Model(Port& port) : port_(port)
            {
                const ParamInfo& p = port.info();
                label_ = upper(p.label.empty() ? p.id : p.label);
                if (label_.size() > kLabelChars)
                    label_.resize(kLabelChars);
                while (!label_.empty() && label_.back() == ' ')
                    label_.pop_back();
                if (discrete(p))
                {
                    const int n = stepsOf(p);
                    names_.reserve(static_cast<std::size_t>(n));
                    for (int i = 0; i < n; ++i)
                    {
                        const auto u = static_cast<std::size_t>(i);
                        if (u < p.choices.size())
                            names_.push_back(p.choices[u]);
                        else if (p.kind == ParamInfo::Kind::toggle)
                            names_.push_back(i == 0 ? "OFF" : "ON");
                        else
                            names_.push_back(std::to_string(static_cast<long>(std::lround(p.min)) + i));
                    }
                    detents_.reserve(names_.size());
                    for (int i = 0; i < n; ++i)
                    {
                        const char* name = names_[static_cast<std::size_t>(i)].c_str();
                        detents_.push_back({ n > 1 ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f, name, name });
                    }
                }
            }

            std::uint64_t key() const override
            {
                const double v = port_.plain();
                std::uint64_t bits = 0;
                std::memcpy(&bits, &v, sizeof bits);
                return bits;
            }

            void view(funkgui::ValueView& v) const override
            {
                const ParamInfo& p = port_.info();
                const double plain = port_.plain();
                v.label = label_.c_str();
                v.trackDefault = toHost01(p, p.def);
                v.atDefault = plain == p.def;
                v.bipolar = !discrete(p) && p.min < 0.0 && p.max > 0.0;
                if (discrete(p))
                {
                    const int n = static_cast<int>(detents_.size());
                    const int at = std::clamp(static_cast<int>(std::lround(plain - p.min)), 0, std::max(n - 1, 0));
                    v.state = funkgui::ValueState::stepped;
                    v.nDetents = n;
                    v.detents = detents_.data();
                    v.detent = at;
                    v.track = n > 0 ? (static_cast<float>(at) + 0.5f) / static_cast<float>(n) : 0.0f;
                    std::snprintf(v.text.value, sizeof v.text.value, "%.9s", n > 0 ? detents_[static_cast<std::size_t>(at)].label : "");
                    std::snprintf(v.text.spoken, sizeof v.text.spoken, "%.60s", v.text.value);
                    return;
                }
                v.state = funkgui::ValueState::continuous;
                v.track = toHost01(p, plain);
                std::snprintf(v.text.value, sizeof v.text.value, "%.*f", std::clamp(p.decimals, 0, 6), plain * p.scale);
                std::snprintf(v.text.unit, sizeof v.text.unit, "%.7s", p.unit.c_str());
                std::snprintf(v.text.spoken, sizeof v.text.spoken, "%s %.20s", v.text.value, p.unit.c_str());
            }

            funkgui::ParamPort* port() override { return &port_; }
            float host01FromTrack(float t) const override { return t; }
            float defaultHost01() const override { return toHost01(port_.info(), port_.info().def); }

        private:
            Port& port_;
            std::string label_;
            std::vector<std::string> names_;
            std::vector<funkgui::Detent> detents_;
        };

        struct ParamDial
        {
            ParamDial(ParamSource& source, int index, funkgui::DialGeom geom, std::uint32_t id)
                : port(source, index), model(port), dial(model, geom, id)
            {
            }

            Port port;
            Model model;
            funkgui::Dial dial;
        };

        // ---- the layout (logical px) --------------------------------------------------------------------------------

        constexpr int kWidth = 904, kHeight = 620;
        constexpr float kMargin = 16.0f;
        constexpr float kGridX = 152.0f, kGridY = 168.0f, kCellW = 72.0f, kRowH = 86.0f;
        constexpr int kColumns = 10;
        constexpr Rect kStart{ 16.0f, 168.0f, 112.0f, 44.0f };
        constexpr Rect kStrip{ 16.0f, 232.0f, 44.0f, 196.0f };
        constexpr Rect kBrakeBar{ 72.0f, 232.0f, 10.0f, 196.0f };
        constexpr Rect kClutchBar{ 94.0f, 232.0f, 10.0f, 196.0f };
        constexpr Rect kRevBar{ 16.0f, 126.0f, 420.0f, 8.0f };
        constexpr float kFooterY = 542.0f;
        constexpr std::uint32_t kIdStart = 1, kIdStrip = 2, kIdFirstDial = 16;

        const char* stateWord(int state) noexcept
        {
            switch (state)
            {
                case 1: return "CRANKING";
                case 2: return "RUNNING";
                case 3: return "STOPPING";
                default: return "OFF";
            }
        }

        class MainPanel final : public funkgui::Panel
        {
        public:
            MainPanel(Player& player, const AppServices& services)
                : params_(player), telemetry_(player), services_(services)
            {
                const int n = params_.count();
                dials_.reserve(static_cast<std::size_t>(n));
                for (int i = 0; i < n; ++i)
                {
                    const funkgui::DialGeom geom{ kGridX + 6.0f + kCellW * static_cast<float>(i % kColumns),
                                                  kGridY + kRowH * static_cast<float>(i / kColumns),
                                                  funkgui::DialSize::primary };
                    dials_.push_back(std::make_unique<ParamDial>(params_, i, geom, kIdFirstDial + static_cast<std::uint32_t>(i)));
                }
                hState_ = telemetry_.findValue(telemetry::kState);
                hSpeed_ = telemetry_.findValue(telemetry::kSpeed);
                hRedline_ = telemetry_.findValue(telemetry::kRedline);
                hThrottle_ = telemetry_.findValue(telemetry::kThrottle);
                hManifold_ = telemetry_.findValue(telemetry::kManifold);
                hCoolant_ = telemetry_.findValue(telemetry::kCoolant);
                hOil_ = telemetry_.findValue(telemetry::kOilPressure);
                hFlags_ = telemetry_.findValue(telemetry::kFlags);
                hCarSpeed_ = telemetry_.findValue(telemetry::kCarSpeed);
                hGear_ = telemetry_.findValue(telemetry::kGear);
                hDriven_ = telemetry_.findValue(telemetry::kDriven);
                hBoost_ = telemetry_.findValue(telemetry::kBoost);
            }

            void attach(funkgui::HostServices& host) override
            {
                host_ = &host;
                gestures_.emplace(host);
            }

            int width() const override { return kWidth; }
            int height() const override { return kHeight; }

            void tick(float dt) override
            {
                for (std::size_t i = 0; i < dials_.size(); ++i)
                {
                    funkgui::Dial& d = dials_[i]->dial;
                    d.tick(dt, hasPointer_ && pressed_ < 0 && !overControls(pointer_) && d.contains(pointer_), false,
                           false);
                }
                // the meter falls slowly, so a short peak can be read
                const float peak = telemetry_.outputPeak();
                meter_ = peak > meter_ ? peak : std::max(peak, meter_ - dt * 0.7f);
                dirty_ = false;
            }

            void idle(double nowSec) override
            {
                if (gestures_)
                    gestures_->poll(nowSec);
            }

            bool wantsFullRate() const override
            {
                if (dirty_ || stripDrag_ || startDown_ || meter_ > 0.0005f)
                    return true;
                if (hState_ >= 0 && std::lround(telemetry_.value(hState_)) != 0)
                    return true;
                const ControlsFrame frame = controls();
                if (frame.throttle > 0.0f || frame.brake > 0.0f || frame.ignition == 2)
                    return true;
                for (const auto& d : dials_)
                    if (!d->dial.settled())
                        return true;
                return false;
            }

            // ---- input ----------------------------------------------------------------------------------------------

            void pointerMove(const funkgui::PointerEvent& e) override
            {
                pointer_ = { e.x, e.y };
                hasPointer_ = true;
                dirty_ = true;
            }

            void pointerExit() override
            {
                hasPointer_ = false;
                dirty_ = true;
            }

            void pointerDown(const funkgui::PointerEvent& e) override
            {
                pointer_ = { e.x, e.y };
                hasPointer_ = true;
                dirty_ = true;
                if (e.popup)
                    return;
                if (kStart.contains(pointer_))
                {
                    startDown_ = true;
                    send();
                    return;
                }
                if (kStrip.expanded(4.0f).contains(pointer_))
                {
                    stripDrag_ = true;
                    stripValue_ = stripAt(e.y);
                    send();
                    return;
                }
                pressed_ = dialAt(pointer_);
                if (pressed_ >= 0 && gestures_ && host_ != nullptr)
                    dials_[static_cast<std::size_t>(pressed_)]->dial.pointerDown(e, *gestures_, *host_);
            }

            void pointerDrag(const funkgui::PointerEvent& e) override
            {
                pointer_ = { e.x, e.y };
                dirty_ = true;
                if (stripDrag_)
                {
                    stripValue_ = stripAt(e.y);
                    send();
                }
                else if (pressed_ >= 0 && gestures_)
                    dials_[static_cast<std::size_t>(pressed_)]->dial.pointerDrag(e, *gestures_);
            }

            void pointerUp(const funkgui::PointerEvent& e) override
            {
                dirty_ = true;
                const bool mine = startDown_ || stripDrag_;
                startDown_ = false;
                if (stripDrag_)
                    stripValue_ = 0.0f;                         // the strip springs back, as a pedal does
                stripDrag_ = false;
                if (mine)
                    send();
                if (pressed_ >= 0 && gestures_)
                    dials_[static_cast<std::size_t>(pressed_)]->dial.pointerUp(e, *gestures_);
                pressed_ = -1;
            }

            void doubleClick(const funkgui::PointerEvent& e) override
            {
                dirty_ = true;
                const int i = dialAt({ e.x, e.y });
                if (i >= 0 && gestures_)
                    dials_[static_cast<std::size_t>(i)]->dial.doubleClick(*gestures_);
            }

            bool wheel(const funkgui::WheelEvent& e) override
            {
                dirty_ = true;
                const Point p{ e.x, e.y };
                if (kStrip.expanded(4.0f).contains(p))
                {
                    // the wheel over the strip sets a throttle that stays (a hand throttle) until it is wheeled to 0
                    // or the strip is pressed
                    stripValue_ = std::clamp(stripValue_ + e.dy * (e.smooth ? 0.25f : 1.0f) * 0.4f, 0.0f, 1.0f);
                    send();
                    return true;
                }
                const int i = dialAt(p);
                if (i < 0 || !gestures_ || host_ == nullptr)
                    return false;
                return dials_[static_cast<std::size_t>(i)]->dial.wheel(e, *gestures_, host_->nowSeconds());
            }

            funkgui::Cursor cursor() const override
            {
                if (!hasPointer_)
                    return funkgui::Cursor::normal;
                if (kStart.contains(pointer_))
                    return funkgui::Cursor::pointingHand;
                if (kStrip.expanded(4.0f).contains(pointer_))
                    return funkgui::Cursor::upDown;
                const int i = dialAt(pointer_);
                return i >= 0 ? dials_[static_cast<std::size_t>(i)]->dial.cursorAt(pointer_) : funkgui::Cursor::normal;
            }

            // ---- accessibility --------------------------------------------------------------------------------------

            void accessibility(std::vector<funkgui::A11yItem>& out) const override
            {
                funkgui::A11yItem start;
                start.id = kIdStart;
                start.role = funkgui::A11yRole::button;
                start.bounds = kStart;
                start.title = "START";
                start.help = "The start key: press to crank, hold a second to switch off";
                out.push_back(start);
                funkgui::A11yItem strip;
                strip.id = kIdStrip;
                strip.role = funkgui::A11yRole::slider;
                strip.bounds = kStrip;
                strip.title = "THROTTLE";
                strip.v = static_cast<double>(controls().throttle);
                strip.step = 0.05;
                out.push_back(strip);
                for (const auto& d : dials_)
                {
                    funkgui::A11yItem item;
                    d->dial.accessibility(item);
                    out.push_back(item);
                }
            }

            std::uint32_t a11yRevision() const override { return 0; }

            void a11yAction(std::uint32_t id, funkgui::A11yAction action, double value) override
            {
                dirty_ = true;
                if (id == kIdStrip && action == funkgui::A11yAction::setValue)
                {
                    stripValue_ = std::clamp(static_cast<float>(value), 0.0f, 1.0f);
                    send();
                    return;
                }
                if (!gestures_)
                    return;
                for (auto& d : dials_)
                    if (d->dial.a11yId() == id)
                        d->dial.a11yAction(action, value, *gestures_);
            }

            void closeGestures() override
            {
                if (gestures_)
                    gestures_->closeAll();
                startDown_ = stripDrag_ = false;
                stripValue_ = 0.0f;
                pressed_ = -1;
                send();
            }

            // ---- drawing --------------------------------------------------------------------------------------------

            void draw(Canvas& c, const Theme& th) override
            {
                const std::string status = services_.status ? services_.status() : std::string();
                const bool failed = status.rfind("error:", 0) == 0;
                drawHeader(c, th, status, failed);
                drawInstruments(c, th);
                drawControls(c, th);
                if (failed && dials_.empty())
                    drawError(c, th, status,
                              { kGridX, kGridY, static_cast<float>(kWidth) - kGridX - kMargin, kFooterY - kGridY - 12.0f }, 20);
                for (const auto& d : dials_)
                    d->dial.draw(c, th, false);
                drawFooter(c, th, status, failed && !dials_.empty());
            }

        private:
            double tel(int handle) const { return handle >= 0 ? telemetry_.value(handle) : 0.0; }

            ControlsFrame controls() const { return services_.controls ? services_.controls() : ControlsFrame{}; }

            // What this Panel's own controls ask for, to the application: the start button, and the throttle strip
            // while it is pressed or wheeled above 0.
            void send()
            {
                if (!services_.panelControls)
                    return;
                PanelControls p;
                p.startHeld = startDown_;
                p.throttle = stripDrag_ || stripValue_ > 0.0f ? stripValue_ : -1.0f;
                services_.panelControls(p);
            }

            static bool overControls(Point p) noexcept
            {
                return kStart.contains(p) || kStrip.expanded(4.0f).contains(p);
            }

            int dialAt(Point p) const
            {
                for (std::size_t i = 0; i < dials_.size(); ++i)
                    if (dials_[i]->dial.contains(p))
                        return static_cast<int>(i);
                return -1;
            }

            static float stripAt(float y) noexcept
            {
                return std::clamp((kStrip.bottom() - y) / kStrip.h, 0.0f, 1.0f);
            }

            void drawHeader(Canvas& c, const Theme& th, const std::string& status, bool failed) const
            {
                c.text("THE ENGINE ENGINE", kMargin, c.capCentreTop(24.0f, type::kWordmark), type::kWordmark, th.ink100);
                // the status' first line (a compile error's other lines go where there is room: drawError)
                const std::string first = status.substr(0, status.find('\n'));
                c.text(fit(c, first, static_cast<float>(kWidth) - 232.0f - kMargin).c_str(), static_cast<float>(kWidth) - kMargin,
                       c.capCentreTop(24.0f, type::kMicro), type::kMicro, failed ? th.accent : th.ink52, funkgui::Align::right);
                c.hairlineH(kMargin, 40.0f, static_cast<float>(kWidth) - 2.0f * kMargin, th.ink16);
            }

            void drawInstruments(Canvas& c, const Theme& th) const
            {
                char text[64];
                const int state = static_cast<int>(std::lround(tel(hState_)));
                const int flags = static_cast<int>(std::lround(tel(hFlags_)));
                const double rpm = tel(hSpeed_) * 9.549296585513721;
                const double redline = tel(hRedline_) * 9.549296585513721;

                c.text("RPM", kMargin, 52.0f, type::kCaption, th.ink52);
                std::snprintf(text, sizeof text, "%.0f", rpm);
                c.text(text, kMargin, 68.0f, type::kDisplay, state == 2 ? th.ink100 : th.ink32);

                // the rev bar: the speed against the redline, with room past it
                const double top = redline > 0.0 ? redline * 1.1 : 8000.0;
                c.rrect(kRevBar.x, kRevBar.y, kRevBar.w, kRevBar.h, 2.0f, th.ink16);
                const float filled = kRevBar.w * static_cast<float>(std::clamp(rpm / top, 0.0, 1.0));
                if (filled > 0.5f)
                    c.rrect(kRevBar.x, kRevBar.y, filled, kRevBar.h, 2.0f, (flags & 1) != 0 ? th.accent : th.ink70);
                if (redline > 0.0)
                    c.hairlineV(kRevBar.x + kRevBar.w * static_cast<float>(redline / top), kRevBar.y - 3.0f, kRevBar.h + 6.0f,
                                th.signal);

                const bool driven = hDriven_ < 0 || tel(hDriven_) > 0.5;
                c.text("GEAR", 240.0f, 52.0f, type::kCaption, th.ink52);
                const int gear = static_cast<int>(std::lround(tel(hGear_)));
                if (!driven || hGear_ < 0)
                    std::snprintf(text, sizeof text, "-");
                else if (gear == 0)
                    std::snprintf(text, sizeof text, "N");
                else if (gear < 0)
                    std::snprintf(text, sizeof text, "R");
                else
                    std::snprintf(text, sizeof text, "%d", gear);
                c.text(text, 240.0f, 68.0f, type::kDisplay, th.ink100);

                c.text("KM/H", 320.0f, 52.0f, type::kCaption, th.ink52);
                std::snprintf(text, sizeof text, "%.0f", driven ? std::fabs(tel(hCarSpeed_)) * 3.6 : 0.0);
                c.text(text, 320.0f, 68.0f, type::kDisplay, driven ? th.ink100 : th.ink32);

                // the small readouts, in two columns
                const float x0 = 470.0f, x1 = 690.0f, valueDx = 96.0f;
                float y = 54.0f;
                const auto row = [&](float x, const char* label, const char* value, Col ink) {
                    c.text(label, x, y, type::kMicro, th.ink52);
                    c.text(value, x + valueDx, y, type::kMicro, ink);
                };
                row(x0, "STATE", stateWord(state), state == 2 ? th.ink100 : th.ink70);
                y += 16.0f;
                std::snprintf(text, sizeof text, "%.0f %%", 100.0 * tel(hThrottle_));
                row(x0, "THROTTLE", text, th.ink100);
                y += 16.0f;
                if (hManifold_ >= 0)
                    std::snprintf(text, sizeof text, "%.2f bar", tel(hManifold_) * 1e-5);
                else
                    std::snprintf(text, sizeof text, "-");
                row(x0, "MANIFOLD", text, th.ink100);
                y += 16.0f;
                if (hBoost_ >= 0)
                    std::snprintf(text, sizeof text, "%.2f bar", tel(hBoost_) * 1e-5);
                else
                    std::snprintf(text, sizeof text, "-");
                row(x0, "BOOST", text, th.ink100);
                y += 16.0f;
                row(x0, "LIMITER", (flags & 1) != 0 ? "ON" : "-", (flags & 1) != 0 ? th.accent : th.ink32);

                y = 54.0f;
                if (hCoolant_ >= 0)
                    std::snprintf(text, sizeof text, "%.0f C", tel(hCoolant_) - 273.15);
                else
                    std::snprintf(text, sizeof text, "-");
                row(x1, "COOLANT", text, th.ink100);
                y += 16.0f;
                if (hOil_ >= 0)
                    std::snprintf(text, sizeof text, "%.1f bar", tel(hOil_) * 1e-5);
                else
                    std::snprintf(text, sizeof text, "-");
                row(x1, "OIL", text, th.ink100);
                y += 16.0f;
                row(x1, "FUEL CUT", (flags & 2) != 0 ? "ON" : "-", (flags & 2) != 0 ? th.ink100 : th.ink32);
                y += 16.0f;
                if (meter_ > 1e-5f)
                    std::snprintf(text, sizeof text, "%.1f dB", 20.0 * std::log10(static_cast<double>(meter_)));
                else
                    std::snprintf(text, sizeof text, "silent");
                row(x1, "OUTPUT", text, meter_ >= 0.999f ? th.accent : th.ink100);
                y += 18.0f;
                // the output's level as a bar, -60 dB to full scale
                const float level = meter_ > 1e-3f ? 1.0f + std::log10(meter_) / 3.0f : 0.0f;
                c.rrect(x1, y, 190.0f, 6.0f, 2.0f, th.ink16);
                if (level > 0.005f)
                    c.rrect(x1, y, 190.0f * std::min(level, 1.0f), 6.0f, 2.0f, meter_ >= 0.999f ? th.accent : th.ink70);

                c.hairlineH(kMargin, 152.0f, static_cast<float>(kWidth) - 2.0f * kMargin, th.ink16);
            }

            void drawControls(Canvas& c, const Theme& th) const
            {
                const ControlsFrame frame = controls();
                const int state = static_cast<int>(std::lround(tel(hState_)));
                const bool startHeld = frame.ignition == 2;

                // the start button
                const bool overStart = hasPointer_ && kStart.contains(pointer_);
                c.rrect(kStart.x, kStart.y, kStart.w, kStart.h, 4.0f, startHeld ? th.accentDim : th.ground.withAlpha(0.0f),
                        overStart || startHeld ? 2.0f : 1.0f, startHeld ? th.accent : (state == 2 ? th.ink100 : th.ink52));
                c.text("START", kStart.centreX(), c.capCentreTop(kStart.centreY(), type::kLatch), type::kLatch,
                       state == 2 || startHeld ? th.ink100 : th.ink70, funkgui::Align::centre);

                // the throttle strip, and the brake and the clutch beside it
                const auto bar = [&](const Rect& r, float v, Col fill) {
                    c.rrect(r.x, r.y, r.w, r.h, 3.0f, th.ink16);
                    const float h = r.h * std::clamp(v, 0.0f, 1.0f);
                    if (h > 0.5f)
                        c.rrect(r.x, r.bottom() - h, r.w, h, 3.0f, fill);
                };
                bar(kStrip, frame.throttle, stripDrag_ ? th.accent : th.ink70);
                bar(kBrakeBar, frame.brake, th.ink52);
                bar(kClutchBar, frame.clutch, th.ink52);
                if (hasPointer_ && kStrip.expanded(4.0f).contains(pointer_))
                    c.rrect(kStrip.x - 2.0f, kStrip.y - 2.0f, kStrip.w + 4.0f, kStrip.h + 4.0f, 5.0f,
                            th.accent.withAlpha(0.0f), 1.0f, th.accent);
                c.text("THR", kStrip.centreX(), kStrip.bottom() + 8.0f, type::kMicro, th.ink52, funkgui::Align::centre);
                c.text("BRK", kBrakeBar.centreX(), kBrakeBar.bottom() + 8.0f, type::kMicro, th.ink52, funkgui::Align::centre);
                c.text("CLU", kClutchBar.centreX() + 6.0f, kClutchBar.bottom() + 8.0f, type::kMicro, th.ink52,
                       funkgui::Align::centre);
            }

            void drawFooter(Canvas& c, const Theme& th, const std::string& status, bool errorHere) const
            {
                c.hairlineH(kMargin, kFooterY - 8.0f, static_cast<float>(kWidth) - 2.0f * kMargin, th.ink16);
                if (errorHere)
                {
                    drawError(c, th, status, { kMargin, kFooterY, static_cast<float>(kWidth) - 2.0f * kMargin, 64.0f }, 4);
                    return;
                }
                // the dial under the pointer in a line, else what to press
                const int over = pressed_ >= 0 ? pressed_ : (hasPointer_ ? dialAt(pointer_) : -1);
                if (over >= 0)
                {
                    char line[256];
                    dials_[static_cast<std::size_t>(over)]->dial.specLine(line, sizeof line);
                    c.text(fit(c, line, static_cast<float>(kWidth) - 2.0f * kMargin).c_str(), kMargin, kFooterY, type::kMicro,
                           th.ink70);
                }
                else
                    c.text("PRESS RETURN (OR CLICK START) TO START THE ENGINE. A DIAL TURNS BY A DRAG UP OR DOWN, OR THE WHEEL.",
                           kMargin, kFooterY, type::kMicro, th.ink52);
                c.text("RETURN START   UP/W THROTTLE   DOWN/S BRAKE   E/Q SHIFT   N NEUTRAL   1-6 GEAR   L-SHIFT CLUTCH",
                       kMargin, kFooterY + 18.0f, type::kMicro, th.ink32);
                c.text("HOLD RETURN A SECOND TO SWITCH OFF   DRAG A DIAL UP OR DOWN   DOUBLE-CLICK A DIAL FOR ITS DEFAULT",
                       kMargin, kFooterY + 34.0f, type::kMicro, th.ink32);
            }

            // The application's error in `box`, a line of its text a row, at most `rows` of them.
            static void drawError(Canvas& c, const Theme& th, const std::string& text, const Rect& box, int rows)
            {
                const funkgui::TextStyle plain{ 11.0f, 0.0f, 0.0f, false };
                float y = box.y;
                std::size_t at = 0;
                for (int row = 0; row < rows && at < text.size() && y + 12.0f <= box.bottom() + 0.5f; ++row)
                {
                    std::size_t end = text.find('\n', at);
                    if (end == std::string::npos)
                        end = text.size();
                    std::string line = text.substr(at, end - at);
                    if (row == rows - 1 && end < text.size())
                        line += " ...";
                    c.text(fit(c, line, box.w, plain).c_str(), box.x, y, plain, th.ink100);
                    y += 14.0f;
                    at = end + 1;
                }
            }

            // `text` cut to whole characters that fit `width`; what the atlas lacks (a tab, a byte past ASCII) becomes
            // a space, so a compiler's message never draws a missing glyph.
            static std::string fit(const Canvas& c, std::string text, float width, const funkgui::TextStyle& style = type::kMicro)
            {
                for (char& ch : text)
                    if (static_cast<unsigned char>(ch) < 32 || static_cast<unsigned char>(ch) > 126)
                        ch = ' ';
                while (!text.empty() && c.textWidth(text.c_str(), style) > width)
                    text.resize(text.size() > 8 ? text.size() - 4 : text.size() - 1);
                return text;
            }

            ParamSource& params_;
            const TelemetrySource& telemetry_;
            AppServices services_;
            funkgui::HostServices* host_ = nullptr;
            std::optional<funkgui::GestureController> gestures_;
            std::vector<std::unique_ptr<ParamDial>> dials_;
            int hState_ = -1, hSpeed_ = -1, hRedline_ = -1, hThrottle_ = -1, hManifold_ = -1, hCoolant_ = -1, hOil_ = -1,
                hFlags_ = -1, hCarSpeed_ = -1, hGear_ = -1, hDriven_ = -1, hBoost_ = -1;
            Point pointer_{};
            bool hasPointer_ = false, dirty_ = true, stripDrag_ = false, startDown_ = false;
            int pressed_ = -1;
            float stripValue_ = 0.0f, meter_ = 0.0f;
        };
    }

    std::unique_ptr<funkgui::Panel> makePlaceholderPanel(tee::Player& player, const tee::AppServices& services)
    {
        return std::make_unique<MainPanel>(player, services);
    }
}
