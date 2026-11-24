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
        if (settings_.autoClutch)
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
        if (action == Action::shiftUp)
        {
            ++shiftUp_;
            seeking_ = false;       // the driver's own shift ends a gear key's
            return;
        }
        if (action == Action::shiftDown)
        {
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

    void Driver::release(Source source)
    {
        if (source >= Source::count)
            return;
        for (std::uint8_t& bits : held_)
            bits = static_cast<std::uint8_t>(bits & ~bitOf(source));
        if (source == Source::gamepad)
            padThrottle_ = padBrake_ = padSteer_ = 0.0f;
        if (source == Source::panel)
            stripHeld_ = brakeStripHeld_ = false;
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

        throttle_ = ease(throttle_, held(Action::throttle), settings_.throttle, dt);
        brake_ = ease(brake_, held(Action::brake), settings_.brake, dt);
        clutch_ = ease(clutch_, held(Action::clutch), settings_.clutch, dt);
        handbrake_ = ease(handbrake_, held(Action::handbrake), settings_.handbrake, dt);

        // the start key (Driver.h)
        const bool start = held(Action::start);
        if (start && !startWas_)
        {
            startOnRunning_ = seen.running;
            startHeld_ = 0.0f;
        }
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
        frame_.throttle = std::max({ throttle_, padThrottle_, stripHeld_ ? strip_ : 0.0f });
        frame_.brake = std::max({ brake_, padBrake_, brakeStripHeld_ ? brakeStrip_ : 0.0f });
        frame_.clutch = clutch_;
        frame_.handbrake = handbrake_;
        frame_.steer = padSteer_;
        frame_.lever = settings_.autoClutch ? 0 : lever_;
        frame_.shiftUp = shiftUp_;
        frame_.shiftDown = shiftDown_;
        frame_.buttons = 0;
        frame_.mode = settings_.mode;
        return frame_;
    }
}
