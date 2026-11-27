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
        carSpeedHandle_ = telemetry.findValue(telemetry::kCarSpeed);
        shiftHandle_ = telemetry.findValue(telemetry::kShift);
        // the build's fields, where the telemetry's source is a Player (the simulator, the reference, the tone: each is a
        // ParamSource too): the gearbox a press drives (Driver::setGearbox)
        params_ = dynamic_cast<const ParamSource*>(&telemetry);
    }

    int Controls::boxKindOf(const ParamSource& params)
    {
        const int model = params.findParam("car.model"), box = params.findParam("car.gearbox");
        if (model < 0 || box < 0 || std::lround(params.get(model)) != 1)
            return -1;
        return static_cast<int>(std::lround(params.get(box)));
    }

    std::string Controls::clutchLine() const
    {
        // one whole sentence each, at most about 90 characters (the SETTINGS row's small line, U1's)
        const ClutchMode m = clutchModeOf(driver_.settings());
        switch (boxKind_)
        {
            case 1: return "Sequential: it works its own clutch; the shifter's gates are sought.";
            case 2: return "Dual clutch: it works its own clutch; the shifter's gates are sought.";
            case 3: return "Automatic: its selector, R N D, whatever this says.";
            case 4: return "CVT: its selector, R N D, whatever this says.";
            case 5: return "Eighteen-speed: the lever is the shifter's and the clutch always yours.";
            default:
                return m == ClutchMode::shifter   ? "Manual: the shifter is the lever, its clutch yours; keys and paddles shift."
                       : m == ClutchMode::automatic ? "Manual: everything shifts for you, the shifter too; no clutch to work."
                                                    : "Manual: the shifter and the keys move the lever; the clutch is yours.";
        }
    }

    Gearbox Controls::gearboxOf(const ParamSource& params)
    {
        // sim/spec/car.fm: `gearbox` is read only with `model` 1 (the vehicle model); sim/vehicle/vspec.fm's VEH_*
        const int model = params.findParam("car.model"), box = params.findParam("car.gearbox");
        if (model < 0 || box < 0 || std::lround(params.get(model)) != 1)
            return Gearbox::manual;                 // the reference's car: a manual's lever
        switch (std::lround(params.get(box)))
        {
            case 1:     // VEH_SEQUENTIAL
            case 2:     // VEH_DUALCLUTCH
                return Gearbox::shifts;
            case 3:     // VEH_AUTOMATIC
            case 4:     // VEH_CVT
                return Gearbox::selector;
            case 5:     // VEH_EIGHTEEN
                return Gearbox::eighteen;
            default: return Gearbox::manual;
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
        if (carSpeedHandle_ >= 0 && driven)
            seen.kmh = static_cast<float>(std::fabs(read(carSpeedHandle_)) * 3.6);
        refused_ = shiftHandle_ >= 0 && driven && std::lround(read(shiftHandle_)) == 3;
        inGearAtRest_ = driven && seen.gearKnown && seen.gear != 0 && seen.kmh < 1.0f && driver_.gateLever() != 0;
        if (params_ != nullptr)
        {
            driver_.setGearbox(gearboxOf(*params_));
            boxKind_ = boxKindOf(*params_);
        }
        sink_->setControls(driver_.advance(dt, seen));
    }

    std::string Controls::status() const
    {
        // what the gears ask of the driver now, first: a gate refused, in gear at rest by the shifter, a gate sought
        std::string ask;
        if (driver_.gateLever() != 0 && refused_)
            ask = "the gear did not go in: the clutch down, or the revs matched";
        else if (inGearAtRest_ && driver_.frame().clutch < 0.5f)
            ask = "in gear (the shifter): the clutch, to pull away";
        else if (driver_.gateSought())
            ask = "the shifter's gates are sought: this wheel shows no clutch pedal";
        if (!ask.empty())
            return status_.empty() ? ask : status_ + "   " + ask;
        // a manual (or the reference's car) with the automatic clutch off and no wheel's pedals: say which key the clutch is
        if ((boxKind_ <= 0) && clutchModeOf(driver_.settings()) == ClutchMode::yours && !driver_.wheelPedals())
        {
            const std::string note = "clutch: yours (Left Shift)";
            return status_.empty() ? note : status_ + "   " + note;
        }
        if (driver_.gearbox() != Gearbox::eighteen)
            return status_;
        const std::string note = "eighteen-speed: the clutch is yours (Left Shift, or the wheel's pedal); start in L "
                                 "(key 5) with the clutch down";
        return status_.empty() ? note : status_ + "   " + note;
    }

    AppServices Controls::services()
    {
        AppServices s;
        s.panelControls = [this](const PanelControls& p) { panel(p); };
        s.controls = [this] { return driver_.frame(); };
        s.status = [this] { return status(); };
        return s;
    }
}
