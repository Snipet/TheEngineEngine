// shell/input/Controls.cpp: Controls.h.
#include "input/Controls.h"

#include <cmath>
#include <utility>

namespace tee::input
{
    Controls::Controls(ControlSink& sink, const TelemetrySource& telemetry)
        : sink_(&sink), telemetry_(&telemetry), bindings_(defaultBindings())
    {
        retarget(sink, telemetry);
    }

    void Controls::retarget(ControlSink& sink, const TelemetrySource& telemetry)
    {
        sink_ = &sink;
        telemetry_ = &telemetry;
        stateHandle_ = telemetry.findValue(telemetry::kState);
        gearHandle_ = telemetry.findValue(telemetry::kGear);
        drivenHandle_ = telemetry.findValue(telemetry::kDriven);
        // the build's fields, where the telemetry's source is a Player (the simulator, the reference, the tone: each is a
        // ParamSource too): the gearbox a press drives (Driver::setGearbox)
        params_ = dynamic_cast<const ParamSource*>(&telemetry);
    }

    Gearbox Controls::gearboxOf(const ParamSource& params)
    {
        // sim/spec/car.fm: `gearbox` is read only with `model` 1 (the vehicle model); sim/vehicle/vspec.fm's VEH_*
        const int model = params.findParam("car.model"), box = params.findParam("car.gearbox");
        if (model < 0 || box < 0 || std::lround(params.get(model)) != 1)
            return Gearbox::shifts;
        switch (std::lround(params.get(box)))
        {
            case 3:     // VEH_AUTOMATIC
            case 4:     // VEH_CVT
                return Gearbox::selector;
            case 5:     // VEH_EIGHTEEN
                return Gearbox::lever;
            default: return Gearbox::shifts;
        }
    }

    bool Controls::key(int key, bool down)
    {
        if (typing_)
            return false;
        const Action action = actionOf(bindings_, key);
        if (action == Action::none)
            return false;
        driver_.set(Source::keyboard, action, down);
        return true;
    }

    void Controls::setTyping(bool typing)
    {
        if (typing && !typing_)
            driver_.release(Source::keyboard);
        typing_ = typing;
    }

    void Controls::focusLost()
    {
        driver_.release(Source::keyboard);
    }

    void Controls::panel(const PanelControls& p)
    {
        driver_.set(Source::panel, Action::start, p.startHeld);
        driver_.setStrip(p.throttle, p.throttle >= 0.0f);
        driver_.setBrakeStrip(p.brake, p.brake >= 0.0f);
        driver_.setClutchStrip(p.clutch, p.clutch >= 0.0f);
        driver_.setHandbrakeStrip(p.handbrake, p.handbrake >= 0.0f);
        // the counts: each increase a press (a count that jumped far is a new Panel's: followed, not pressed)
        const auto presses = [this](int now, int& seen, Action action) {
            const int more = now - seen;
            seen = now;
            if (more > 0 && more <= 8)
                for (int i = 0; i < more; ++i)
                    driver_.press(action);
        };
        presses(p.shiftUp, panelUp_, Action::shiftUp);
        presses(p.shiftDown, panelDown_, Action::shiftDown);
        if (p.leverSet && (!panelLeverSet_ || p.lever != panelLever_))
            driver_.requestGear(p.lever);
        panelLeverSet_ = p.leverSet;
        panelLever_ = p.lever;
    }

    void Controls::tick(float dt)
    {
        // (a value not there yet, NaN while the engine compiles, reads as 0)
        const auto read = [this](int handle) {
            const double v = telemetry_->value(handle);
            return std::isfinite(v) ? v : 0.0;
        };
        EngineSeen seen;
        if (stateHandle_ >= 0)
            seen.running = std::lround(read(stateHandle_)) == 2;
        // a gearbox answers only while the engine is in a vehicle
        const bool driven = drivenHandle_ < 0 || read(drivenHandle_) > 0.5;
        if (gearHandle_ >= 0 && driven)
        {
            seen.gearKnown = true;
            seen.gear = static_cast<int>(std::lround(read(gearHandle_)));
        }
        if (params_ != nullptr)
            driver_.setGearbox(gearboxOf(*params_));
        sink_->setControls(driver_.advance(dt, seen));
    }

    AppServices Controls::services()
    {
        AppServices s;
        s.panelControls = [this](const PanelControls& p) { panel(p); };
        s.controls = [this] { return driver_.frame(); };
        s.status = [this] { return status_; };
        return s;
    }
}
