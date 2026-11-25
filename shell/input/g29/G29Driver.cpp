// shell/input/g29/G29Driver.cpp: G29Driver.h. Ported from TestingPlayground's Source/plugin/WheelDriver.cpp (47bac40;
// SEED.tsv), the MIDI taken out.
#include "input/g29/G29Driver.h"

#include <algorithm>
#include <cstdio>

namespace tee::input::g29
{
    namespace
    {
        std::string hex(int r)
        {
            char b[16];
            std::snprintf(b, sizeof b, "0x%08x", static_cast<unsigned>(r));
            return b;
        }

        constexpr const char* kWhatToDo = ": plug it in again, then turn this off and on";
    }

    std::string Transport::describe(int error) { return hex(error); }

    const Driver::Known* Driver::find(Kind kind) const noexcept
    {
        for (const Known& k : devices_)
            if (k.kind == kind)
                return &k;
        return nullptr;
    }

    bool Driver::has(Device d) const noexcept
    {
        return std::any_of(devices_.begin(), devices_.end(), [d](const Known& k) { return k.d == d; });
    }

    bool Driver::present() const noexcept { return !devices_.empty() || (on_ && switch_ != Switch::none); }

    std::string Driver::status() const
    {
        if (!present())
            return {};
#if defined(__APPLE__)
        if (opened_ == kNotPermitted)
            return "macOS keeps it from this app: allow it in Privacy & Security, Input Monitoring";
        if (opened_ != 0)
            return "macOS could not open it (" + t_.describe(opened_) + ")";
#else
        if (opened_ == kNotPermitted)
            return "the system keeps it from this app";
        if (opened_ != 0)
            return "it could not be opened (" + t_.describe(opened_) + ")";
#endif
        if (find(Kind::g29) != nullptr)
            return problem_;                                 // on: its range and spring; off: its spring's stop
        if (on_ && switch_ == Switch::failed)
            return (problem_.empty() ? std::string("it did not switch to its full mode") : problem_) + kWhatToDo;
        if (on_ && switch_ == Switch::pending)
            return "switching to its full mode";
        if (find(Kind::compat) != nullptr)
            return on_ ? "back in its PS3 mode: turn this off and on to switch it"
                       : "in its PS3 mode: switched to its full mode when on";
        return "set the switch on the wheel to PS3";
    }

    void Driver::opened(int result)
    {
        opened_ = result;
        refresh();
    }

    void Driver::added(Device d, Kind kind)
    {
        if (quit_ || kind == Kind::none || has(d))
            return;
        devices_.push_back({ d, kind });
        if (kind == Kind::g29)
        {
            problem_.clear();
            endSwitch(Switch::none);                         // the switch worked (or it came so)
        }
        else if (kind == Kind::compat && switch_ == Switch::pending)
            endSwitch(Switch::failed);                       // back in its compatibility mode: the switch did not take
        refresh();
    }

    void Driver::removed(Device d)
    {
        const auto it = std::find_if(devices_.begin(), devices_.end(), [d](const Known& k) { return k.d == d; });
        if (quit_ || it == devices_.end())
            return;
        if (reading_ == d)
            stopReading(false);                              // its spring went with it
        devices_.erase(it);
        refresh();                                           // a pending switch stays: it re-enumerates
    }

    bool Driver::take(const Value& v)
    {
        std::int64_t x = v.value, lo = v.min, hi = v.max;
        normalise(x, lo, hi, v.reportSize);
        return map_.input(v.page, v.usage, x, lo, hi);
    }

    void Driver::value(Device d, const Value& v)
    {
        if (quit_ || reading_ == nullptr || d != reading_)
            return;
        take(v);
    }

    void Driver::fire(Timer t)
    {
        if (quit_)
            return;
        if (t == Timer::lights)
        {
            if (reading_ != nullptr)
            {
                pollLights();
                t_.arm(Timer::lights, kLightsInterval);
            }
        }
        else if (switch_ == Switch::pending)
        {
            endSwitch(Switch::failed);                       // kSwitchSeconds and no G29 came
            t_.changed();
        }
    }

    // hid-lg4ff's lg4ff_set_range_g25, then lg4ff_set_autocenter_default; then the values the device has reported (never
    // IOKit's cache before a report: the seed rule).
    void Driver::startReading(Device d)
    {
        problem_.clear();
        int r = t_.setRange(d, kRange);
        if (r == 0)
            r = t_.setSpring(d, kSpring);
        if (r != 0)
            problem_ = "its range and spring were not set (" + t_.describe(r) + ")";
        reading_ = d;
        map_.rest();
        for (const Value& v : t_.values(d))
            if (v.timestamp != 0)
                take(v);
        t_.listen(d);
        lightsSent_ = 0;                                     // none lit, as the wheel starts
        pollLights();
        t_.arm(Timer::lights, kLightsInterval);
    }

    void Driver::pollLights()
    {
        const int want = std::clamp(t_.lights(), 0, kAllLights);
        if (reading_ == nullptr || want == lightsSent_)
            return;
        lightsSent_ = want;                                  // a failure is not tried again until the mask changes
        if (const int r = t_.setLights(reading_, want); r != 0 && problem_.empty())
        {
            problem_ = "its shift lights were not set (" + t_.describe(r) + ")";
            t_.changed();
        }
    }

    void Driver::lightsOff()
    {
        if (reading_ != nullptr && lightsSent_ != 0)
            t_.setLights(reading_, 0);
        lightsSent_ = 0;
    }

    void Driver::stopReading(bool stillThere)
    {
        if (reading_ == nullptr)
            return;
        t_.listen(nullptr);
        problem_.clear();
        if (stillThere)
        {
            if (const int r = t_.setSpring(reading_, 0); r != 0)
                problem_ = "its spring may still be on (" + t_.describe(r) + "): unplug it to stop it";
            lightsOff();
        }
        lightsSent_ = 0;                                     // unplugged: they went with it
        t_.disarm(Timer::lights);
        reading_ = nullptr;
        map_.rest();
    }

    void Driver::switchToNative(Device d)
    {
        switchUsed_ = true;
        if (const int r = t_.toNative(d); r != 0)
        {
            problem_ = "it could not be switched to its full mode (" + t_.describe(r) + ")";
            switch_ = Switch::failed;
            return;
        }
        switch_ = Switch::pending;
        t_.arm(Timer::switched, kSwitchSeconds);
    }

    void Driver::endSwitch(Switch to)
    {
        if (switch_ == Switch::pending)
            t_.disarm(Timer::switched);
        switch_ = to;
    }

    void Driver::refresh()
    {
        const Known* g29 = find(Kind::g29);
        if (on_ && g29 != nullptr)
        {
            if (reading_ != g29->d)
            {
                stopReading(true);                           // another G29 in its place (still connected)
                startReading(g29->d);
            }
        }
        else
            stopReading(reading_ != nullptr && has(reading_));
        if (on_ && g29 == nullptr && opened_ == 0 && !switchUsed_)
            if (const Known* c = find(Kind::compat))
                switchToNative(c->d);
        t_.changed();
    }

    void Driver::setOn(bool on)
    {
        if (quit_ || on == on_)
            return;
        on_ = on;
        problem_.clear();
        switchUsed_ = false;                                 // switched on: one switch again; off: none until then
        endSwitch(Switch::none);
        refresh();
    }

    void Driver::quit()
    {
        if (quit_)
            return;
        quit_ = true;
        if (reading_ != nullptr)
        {
            t_.listen(nullptr);
            if (const int r = t_.setSpring(reading_, 0); r != 0)       // the wheel turns freely again
                problem_ = "its spring may still be on (" + t_.describe(r) + "): unplug it to stop it";
            lightsOff();
            reading_ = nullptr;
        }
        t_.disarm(Timer::switched);
        t_.disarm(Timer::lights);
        map_.rest();
    }
}
