// ui/kit/gui/Ports.cpp: see Ports.h.
#include "kit/gui/Ports.h"

#include "kit/gui/PanelSpec.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tee::ui
{
    Control controlOf(const ParamInfo& info)
    {
        Control c;
        std::snprintf(c.name, sizeof c.name, "%s", info.id.c_str());
        switch (info.kind)
        {
            case ParamInfo::Kind::real:    c.type = ControlType::real; break;
            case ParamInfo::Kind::integer: c.type = ControlType::integer; break;
            case ParamInfo::Kind::choice:  c.type = ControlType::integer; break;
            case ParamInfo::Kind::toggle:  c.type = ControlType::toggle; break;
        }
        c.min = static_cast<float>(info.min);
        c.max = static_cast<float>(info.max);
        if (c.type == ControlType::toggle)
        {
            c.min = 0.0f;
            c.max = 1.0f;
        }
        c.def = std::clamp(static_cast<float>(info.def), c.min, std::max(c.min, c.max));
        c.used = true;
        c.taper = info.logTaper ? ControlTaper::log : ControlTaper::linear;
        std::snprintf(c.unit, sizeof c.unit, "%s", cutText(info.unit, sizeof c.unit - 1).c_str());
        c.decimals = static_cast<int8_t>(std::clamp(info.decimals, -1, 4));
        c.scale = info.scale != 0.0 ? static_cast<float>(info.scale) : 1.0f;
        return c;
    }

    std::vector<std::string> optionsOf(const ParamInfo& info)
    {
        std::vector<std::string> out;
        if (info.kind != ParamInfo::Kind::choice)
            return out;
        for (const std::string& name : info.choices)
            out.push_back(cutText(name, kMaxOptionLabel));
        return out;
    }

    SourcePort::SourcePort(ParamSource& source, int index)
        : source_(source), sink_(dynamic_cast<GestureSink*>(&source)), index_(index), control_(controlOf(source.info(index))), id_(source.info(index).id)
    {
    }

    float SourcePort::value01() const { return normOf(control_, static_cast<float>(source_.get(index_))); }
    float SourcePort::default01() const { return normOf(control_, control_.def); }

    int SourcePort::numSteps() const
    {
        if (control_.type == ControlType::real)
            return 0;
        return std::max(1, static_cast<int>(std::lround(control_.max - control_.min)));
    }

    void SourcePort::beginGesture()
    {
        ++gestures_;
        if (sink_ != nullptr)
            sink_->beginGesture(index_);
    }

    void SourcePort::endGesture()
    {
        ++ended_;
        if (sink_ != nullptr)
            sink_->endGesture(index_);
    }

    void SourcePort::setValue01(float v) { source_.set(index_, static_cast<double>(plainOf(control_, v))); }
}
