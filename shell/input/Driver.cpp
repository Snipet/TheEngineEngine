// shell/input/Driver.cpp: Driver.h's rules.
#include "input/Driver.h"

#include <algorithm>
#include <cmath>

namespace tee::input
{
    namespace
    {
        float clamp01(float v) noexcept
        {
            return !(v > 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);   // a NaN is 0
        }

        constexpr std::uint8_t bitOf(Source s) noexcept
        {
            return static_cast<std::uint8_t>(1u << static_cast<unsigned>(s));
        }

        constexpr int kNoGear = 99;

        // The gear a gear key names; kNoGear for a key that is none.
        int gearOf(Action a) noexcept
        {
            switch (a)
            {
                case Action::neutral: return 0;
                case Action::reverse: return -1;
                case Action::gear1: return 1;
                case Action::gear2: return 2;
                case Action::gear3: return 3;
                case Action::gear4: return 4;
                case Action::gear5: return 5;
                case Action::gear6: return 6;
                default: return kNoGear;
            }
        }
    }

    Driver::Driver(const DriverSettings& s) : settings_(s) {}

    void Driver::setSettings(const DriverSettings& s)
    {
        settings_ = s;
        if (settings_.autoClutch && gearbox_ == Gearbox::shifts)
            lever_ = 0;             // the lever goes back to neutral: the gearbox keeps the gear it is in
        else
            seeking_ = false;
    }

    bool Driver::held(Action a) const noexcept
    {
        return held_[static_cast<std::size_t>(a)] != 0;
    }

    void Driver::set(Source source, Action action, bool down)
    {
        if (action == Action::none || action >= Action::count || source >= Source::count)
            return;
        std::uint8_t& bits = held_[static_cast<std::size_t>(action)];
        const bool was = bits != 0;
        if (down)
            bits = static_cast<std::uint8_t>(bits | bitOf(source));
        else
            bits = static_cast<std::uint8_t>(bits & ~bitOf(source));
        if (down && !was)
            pressed(action);
    }

    // An action went from up to down: what a press does at once (what a hold does is advance()'s).
    void Driver::pressed(Action action)
    {
        // the truck's switches: latching, a press each turns one on or off (sim/spec/controls.fm, BTN_*)
        if (action == Action::splitter || action == Action::range || action == Action::engineBrake)
        {
            const int bit = action == Action::splitter ? kButtonSplitter : action == Action::range ? kButtonRange : kButtonEngineBrake;
            latched_ ^= bit;
            return;
        }
        if (action == Action::start)
        {
            if (!startEnabled_)
                return;
            startLatch_ = settings_.startLeast;
            startLatchFresh_ = true;    // the next advance's dt is time before the press: it does not count
            return;
        }
        if (action == Action::shiftUp)
        {
            if (gearbox_ == Gearbox::selector)
                select_ = std::min(select_ + 1, 1);
            else if (gearbox_ == Gearbox::eighteen)
                eighteenStep(1);
            ++shiftUp_;
            seeking_ = false;       // the driver's own shift ends a gear key's
            return;
        }
        if (action == Action::shiftDown)
        {
            if (gearbox_ == Gearbox::selector)
            {
                if (select_ != 1 || kmh_ < kCrawlKmh)       // out of D only at a crawl: never into N at speed
                    select_ = std::max(select_ - 1, -1);
            }
            else if (gearbox_ == Gearbox::eighteen)
                eighteenStep(-1);
            ++shiftDown_;
            seeking_ = false;
            return;
        }
        const int gear = gearOf(action);
        if (gear != kNoGear)
            requestGear(gear);
    }

    void Driver::press(Action action)
    {
        if (action != Action::none && action < Action::count)
            pressed(action);
    }

    void Driver::requestGear(int gear)
    {
        gear = std::clamp(gear, -1, 6);
        if (gearbox_ == Gearbox::selector)
        {
            select_ = gear > 0 ? 1 : gear < 0 ? -1 : 0;     // a forward gate is D
            seeking_ = false;
            return;
        }
        if (gearbox_ == Gearbox::eighteen)
        {
            if (gear <= 5)                                   // 1 to 4 the positions, 5 the L slot, 6 nothing
                lever_ = gear;
            seeking_ = false;
            return;
        }
        if (settings_.autoClutch)
        {
            seek_ = gear;
            seeking_ = true;
            seekWait_ = 0.0f;       // the first press goes with the next frame
            seekLeft_ = settings_.seekTries;
        }
        else
            lever_ = gear;
    }

    void Driver::setAnalog(float throttle, float brake, float steer)
    {
        padThrottle_ = clamp01(throttle);
        padBrake_ = clamp01(brake);
        padSteer_ = !(steer >= -1.0f) ? (steer < -1.0f ? -1.0f : 0.0f) : (steer > 1.0f ? 1.0f : steer);
    }

    void Driver::setStrip(float value, bool isHeld)
    {
        strip_ = clamp01(value);
        stripHeld_ = isHeld;
    }

    void Driver::setBrakeStrip(float value, bool isHeld)
    {
        brakeStrip_ = clamp01(value);
        brakeStripHeld_ = isHeld;
    }

    void Driver::setAnalogClutch(float clutch)
    {
        padClutch_ = clamp01(clutch);
    }

    void Driver::setWheel(float throttle, float brake, float clutch)
    {
        wheelThrottle_ = clamp01(throttle);
        wheelBrake_ = clamp01(brake);
        wheelClutch_ = clamp01(clutch);
    }

    void Driver::setClutchStrip(float value, bool isHeld)
    {
        clutchStrip_ = clamp01(value);
        clutchStripHeld_ = isHeld;
    }

    void Driver::setHandbrakeStrip(float value, bool isHeld)
    {
        handbrakeStrip_ = clamp01(value);
        handbrakeStripHeld_ = isHeld;
    }

    void Driver::setStartEnabled(bool on) noexcept
    {
        startEnabled_ = on;
        if (!on)
        {
            startLatch_ = 0.0f;
            crankLeft_ = 0.0f;
        }
    }

    void Driver::setMode(int mode)
    {
        settings_.mode = mode;
    }

    namespace
    {
        constexpr int kLeverL = 5;                  // roadranger.fm's RR_LEVER_L: the low-low slot (gears 1 and 2)
        // roadranger.fm's gearOf: the gear a lever position, range and split make (0: none)
        int eighteenGear(int lever, int range, int split)
        {
            if (lever == kLeverL)
                return range == 0 ? 1 + split : 0;
            if (lever < 1 || lever > 4)
                return 0;
            return range == 0 ? 1 + 2 * lever + split : 9 + 2 * lever + split;
        }
    }

    void Driver::eighteenStep(int by)
    {
        const int range = (latched_ & kButtonRange) != 0 ? 1 : 0, split = (latched_ & kButtonSplitter) != 0 ? 1 : 0;
        const int now = eighteenGear(lever_, range, split);
        // from neutral (or reverse) a step up is the first gear; a step down nothing
        const int want = now == 0 ? (by > 0 ? 1 : 0) : now + by;
        if (want < 1 || want > 18)
            return;                                 // refused at the ends
        int lever = 0, r = 0, s = 0;
        if (want <= 2)
        {
            lever = kLeverL;
            s = want - 1;
        }
        else if (want <= 10)
        {
            lever = (want - 3) / 2 + 1;
            s = (want - 3) % 2;
        }
        else
        {
            lever = (want - 11) / 2 + 1;
            s = (want - 11) % 2;
            r = 1;
        }
        lever_ = lever;
        latched_ = (latched_ & ~(kButtonRange | kButtonSplitter)) | (r != 0 ? kButtonRange : 0) | (s != 0 ? kButtonSplitter : 0);
    }

    void Driver::setGearbox(Gearbox g)
    {
        if (g == gearbox_)
            return;
        gearbox_ = g;
        select_ = 0;
        lever_ = 0;
        seeking_ = false;
    }

    void Driver::release(Source source)
    {
        if (source >= Source::count)
            return;
        for (std::uint8_t& bits : held_)
            bits = static_cast<std::uint8_t>(bits & ~bitOf(source));
        if (source == Source::gamepad)
            padThrottle_ = padBrake_ = padSteer_ = padClutch_ = 0.0f;
        if (source == Source::wheel)
            wheelThrottle_ = wheelBrake_ = wheelClutch_ = 0.0f;
        if (source == Source::panel)
            stripHeld_ = brakeStripHeld_ = clutchStripHeld_ = handbrakeStripHeld_ = false;
    }

    float Driver::ease(float x, bool down, const Easing& e, float dt) noexcept
    {
        const float time = down ? e.rise : e.fall;
        if (!(time > 0.0f))
            return down ? 1.0f : 0.0f;
        const float step = dt / time;
        return down ? std::min(1.0f, x + step) : std::max(0.0f, x - step);
    }

    const ControlsFrame& Driver::advance(float dt, const EngineSeen& seen)
    {
        if (!(dt > 0.0f))
            dt = 0.0f;
        dt = std::min(dt, 0.25f);   // a frame that took long does not slam a pedal
        kmh_ = seen.kmh;

        throttle_ = ease(throttle_, held(Action::throttle), settings_.throttle, dt);
        brake_ = ease(brake_, held(Action::brake), settings_.brake, dt);
        clutch_ = ease(clutch_, held(Action::clutch), settings_.clutch, dt);
        handbrake_ = ease(handbrake_, held(Action::handbrake), settings_.handbrake, dt);

        // the start key (Driver.h)
        // a press on a stopped engine cranks it until it runs (at most crankMax seconds), as docs/controls.md says:
        // a tap is enough, and a key let go early (the window losing the keyboard as it opens) does not cut it short
        if (seen.running)
            crankLeft_ = 0.0f;
        const bool start = startEnabled_ && (held(Action::start) || startLatch_ > 0.0f || crankLeft_ > 0.0f);
        if (startLatchFresh_)
            startLatchFresh_ = false;
        else
            startLatch_ = std::max(0.0f, startLatch_ - dt);
        if (start && !startWas_)
        {
            startOnRunning_ = seen.running;
            startHeld_ = 0.0f;
            if (!seen.running)
                crankLeft_ = settings_.crankMax + dt;     // (this frame's dt is mostly before the press)
        }
        crankLeft_ = std::max(0.0f, crankLeft_ - dt);
        if (start)
        {
            startHeld_ += dt;
            ignition_ = 2;
        }
        else if (startWas_)
            ignition_ = startOnRunning_ && startHeld_ >= settings_.offHold ? 0 : 1;
        startWas_ = start;

        // a gear key, with the automatic clutch: a press of the shift at a time until the gearbox is there
        if (seeking_)
        {
            if (!seen.gearKnown || seen.gear == seek_)
                seeking_ = false;
            else
            {
                seekWait_ -= dt;
                if (seekWait_ <= 0.0f)
                {
                    if (seekLeft_ <= 0)
                        seeking_ = false;
                    else
                    {
                        if (seen.gear < seek_)
                            ++shiftUp_;
                        else
                            ++shiftDown_;
                        --seekLeft_;
                        seekWait_ = settings_.seekInterval;
                    }
                }
            }
        }

        frame_.ignition = ignition_;
        frame_.throttle = std::max({ throttle_, padThrottle_, wheelThrottle_, stripHeld_ ? strip_ : 0.0f });
        frame_.brake = std::max({ brake_, padBrake_, wheelBrake_, brakeStripHeld_ ? brakeStrip_ : 0.0f });
        frame_.clutch = std::max({ clutch_, padClutch_, wheelClutch_, clutchStripHeld_ ? clutchStrip_ : 0.0f });
        frame_.handbrake = std::max(handbrake_, handbrakeStripHeld_ ? handbrakeStrip_ : 0.0f);
        frame_.steer = padSteer_;
        frame_.lever = gearbox_ == Gearbox::selector ? select_
                     : gearbox_ == Gearbox::eighteen ? lever_
                     : settings_.autoClutch        ? 0
                                                   : lever_;
        frame_.shiftUp = shiftUp_;
        frame_.shiftDown = shiftDown_;
        frame_.buttons = latched_;
        frame_.mode = settings_.mode;
        return frame_;
    }
}
