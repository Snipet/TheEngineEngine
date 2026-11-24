// shell/input/WheelMap.cpp: WheelMap.h.
#include "input/WheelMap.h"

#include "input/Driver.h"

#include <algorithm>
#include <cmath>

namespace tee::input
{
    WheelMap WheelMap::typical(std::string device)
    {
        WheelMap m;
        m.device = std::move(device);
        m.axes[static_cast<std::size_t>(AxisRole::steer)] = { 0, 0.0f, 1.0f, 0.0f };
        m.axes[static_cast<std::size_t>(AxisRole::throttle)] = { 1, 1.0f, -1.0f, 0.03f };
        m.axes[static_cast<std::size_t>(AxisRole::brake)] = { 2, 1.0f, -1.0f, 0.03f };
        m.axes[static_cast<std::size_t>(AxisRole::clutch)] = { 3, 1.0f, -1.0f, 0.03f };
        m.buttons = { { 4, Action::shiftUp }, { 5, Action::shiftDown }, { 9, Action::start } };
        return m;
    }

    float axisValue(const AxisMap& a, AxisRole role, const WheelState& s) noexcept
    {
        if (a.axis < 0 || a.axis >= static_cast<int>(s.axes.size()))
            return 0.0f;
        const float raw = s.axes[static_cast<std::size_t>(a.axis)];
        const float span = a.floor - a.rest;
        if (!std::isfinite(raw) || std::fabs(span) < 1e-6f)
            return 0.0f;
        float x = (raw - a.rest) / span;            // 0 at rest, 1 at the floor (the steering: 1 full right)
        if (role == AxisRole::steer)
        {
            // the steering is symmetric about its rest: -1 full left
            x = std::clamp(x, -1.0f, 1.0f);
            return std::fabs(x) < a.deadZone ? 0.0f : x;
        }
        x = std::clamp(x, 0.0f, 1.0f);
        if (x < a.deadZone)
            return 0.0f;
        return (x - a.deadZone) / (1.0f - a.deadZone);
    }

    void applyWheel(const WheelMap& m, const WheelState& now, const WheelState& previous, Driver& d)
    {
        const auto value = [&](AxisRole r) { return axisValue(m.axes[static_cast<std::size_t>(r)], r, now); };
        d.setAnalog(value(AxisRole::throttle), value(AxisRole::brake), value(AxisRole::steer));
        d.setAnalogClutch(value(AxisRole::clutch));
        for (const ButtonMap& b : m.buttons)
        {
            if (b.button < 0 || b.action == Action::none)
                continue;
            const auto i = static_cast<std::size_t>(b.button);
            const bool down = i < now.buttons.size() && now.buttons[i];
            const bool was = i < previous.buttons.size() && previous.buttons[i];
            if (down != was)
                d.set(Source::gamepad, b.action, down);
        }
    }

    void Learn::beginAxis(AxisRole role, const WheelState& now)
    {
        active_ = true;
        axis_ = true;
        role_ = role;
        start_ = now;
    }

    void Learn::beginButton(Action action, const WheelState& now)
    {
        active_ = true;
        axis_ = false;
        action_ = action;
        start_ = now;
    }

    bool Learn::watch(const WheelState& now, WheelMap& map)
    {
        if (!active_)
            return false;
        if (axis_)
        {
            // the axis that moved furthest from where it was, once it has moved more than half its range
            int best = -1;
            float moved = 0.0f;
            for (std::size_t i = 0; i < now.axes.size() && i < start_.axes.size(); ++i)
            {
                const float m = std::fabs(now.axes[i] - start_.axes[i]);
                if (std::isfinite(m) && m > moved)
                {
                    moved = m;
                    best = static_cast<int>(i);
                }
            }
            if (best < 0 || moved < 0.5f)
                return false;
            AxisMap& a = map.axes[static_cast<std::size_t>(role_)];
            a.axis = best;
            const float rest = start_.axes[static_cast<std::size_t>(best)];
            const float direction = now.axes[static_cast<std::size_t>(best)] > rest ? 1.0f : -1.0f;
            if (role_ == AxisRole::steer)
            {
                // learned by turning right: the rest is the centre, the floor full right
                a.rest = rest;
                a.floor = rest + direction * 1.0f;
                a.deadZone = 0.0f;
            }
            else
            {
                // a pedal rests at one end and goes to the other
                a.rest = rest;
                a.floor = direction > 0.0f ? 1.0f : -1.0f;
                if (std::fabs(a.floor - a.rest) < 0.5f)
                    a.floor = rest + direction * 1.0f;
            }
            active_ = false;
            return true;
        }
        for (std::size_t i = 0; i < now.buttons.size(); ++i)
        {
            const bool was = i < start_.buttons.size() && start_.buttons[i];
            if (now.buttons[i] && !was)
            {
                map.buttons.erase(std::remove_if(map.buttons.begin(), map.buttons.end(),
                                                 [&](const ButtonMap& b) {
                                                     return b.button == static_cast<int>(i) || b.action == action_;
                                                 }),
                                  map.buttons.end());
                map.buttons.push_back({ static_cast<int>(i), action_ });
                active_ = false;
                return true;
            }
        }
        return false;
    }
}
