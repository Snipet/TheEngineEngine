// ui/kit/bind/Fields.cpp: see Fields.h.
#include "kit/bind/Fields.h"

#include <algorithm>
#include <cmath>

namespace tee::ui
{
    int FieldDoc::find(std::string_view path) const
    {
        for (int i = 0; i < count(); ++i)
            if (info(i).path == path)
                return i;
        return -1;
    }

    std::vector<std::string> sectionsOf(const FieldDoc& doc)
    {
        std::vector<std::string> out;
        for (int i = 0; i < doc.count(); ++i)
        {
            const std::string s(doc.info(i).section());
            if (std::find(out.begin(), out.end(), s) == out.end())
                out.push_back(s);
        }
        return out;
    }

    double clampTo(const FieldInfo& f, double v) noexcept
    {
        if (!std::isfinite(v))
            return f.def;
        switch (f.kind)
        {
            case FieldKind::toggle: return v >= 0.5 ? 1.0 : 0.0;
            case FieldKind::integer:
            case FieldKind::choice: return std::clamp(std::round(v), f.min, std::max(f.min, f.max));
            case FieldKind::number:
            case FieldKind::curve:
            case FieldKind::map: break;
        }
        return std::clamp(v, f.min, std::max(f.min, f.max));
    }

    MemoryDoc::MemoryDoc(std::vector<FieldInfo> fields) : fields_(std::move(fields))
    {
        const std::size_t n = fields_.size();
        now_.values.resize(n);
        now_.curves.resize(n);
        now_.maps.resize(n);
        reset();
        undo_.clear();
    }

    void MemoryDoc::reset()
    {
        beginGesture();
        for (std::size_t i = 0; i < fields_.size(); ++i)
        {
            const FieldInfo& f = fields_[i];
            now_.values[i] = clampTo(f, f.def);
            if (f.kind == FieldKind::curve)          // a plain ramp across x, at the range's middle
            {
                Curve16 c;
                c.n = 8;
                for (int k = 0; k < c.n; ++k)
                {
                    const double t = static_cast<double>(k) / (c.n - 1);
                    c.x[static_cast<std::size_t>(k)] = f.axisLo[0] + (f.axisHi[0] - f.axisLo[0]) * t;
                    c.y[static_cast<std::size_t>(k)] = f.min + (f.max - f.min) * (0.25 + 0.5 * t);
                }
                now_.curves[i] = c;
            }
            if (f.kind == FieldKind::map)
            {
                Map16 m;
                m.nx = m.ny = 8;
                for (int k = 0; k < 8; ++k)
                {
                    const double t = static_cast<double>(k) / 7.0;
                    m.x[static_cast<std::size_t>(k)] = f.axisLo[0] + (f.axisHi[0] - f.axisLo[0]) * t;
                    m.y[static_cast<std::size_t>(k)] = f.axisLo[1] + (f.axisHi[1] - f.axisLo[1]) * t;
                }
                for (int j = 0; j < 8; ++j)
                    for (int k = 0; k < 8; ++k)
                        m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(k)] = f.def;
                now_.maps[i] = m;
            }
        }
        changed();
        endGesture();
    }

    void MemoryDoc::changed() { ++revision_; }

    void MemoryDoc::set(int i, double si)
    {
        const double v = clampTo(fields_[static_cast<std::size_t>(i)], si);
        if (v == now_.values[static_cast<std::size_t>(i)])
            return;
        beginGesture();
        now_.values[static_cast<std::size_t>(i)] = v;
        changed();
        endGesture();
    }

    void MemoryDoc::setCurve(int i, const Curve16& c)
    {
        const FieldInfo& f = fields_[static_cast<std::size_t>(i)];
        Curve16 k = c;
        k.n = std::clamp(k.n, 0, kTableMax);
        for (std::size_t p = 0; p < kTableMax; ++p)
            k.y[p] = std::clamp(k.y[p], f.min, f.max);
        if (k == now_.curves[static_cast<std::size_t>(i)])
            return;
        beginGesture();
        now_.curves[static_cast<std::size_t>(i)] = k;
        changed();
        endGesture();
    }

    void MemoryDoc::setMap(int i, const Map16& m)
    {
        const FieldInfo& f = fields_[static_cast<std::size_t>(i)];
        Map16 k = m;
        k.nx = std::clamp(k.nx, 0, kTableMax);
        k.ny = std::clamp(k.ny, 0, kTableMax);
        for (auto& row : k.z)
            for (double& z : row)
                z = std::clamp(z, f.min, f.max);
        if (k == now_.maps[static_cast<std::size_t>(i)])
            return;
        beginGesture();
        now_.maps[static_cast<std::size_t>(i)] = k;
        changed();
        endGesture();
    }

    void MemoryDoc::beginGesture()
    {
        if (depth_++ == 0)
            before_ = now_;
    }

    void MemoryDoc::endGesture()
    {
        if (depth_ <= 0)
            return;
        if (--depth_ == 0 && !(before_ == now_))
        {
            undo_.push_back(std::move(before_));
            redo_.clear();
        }
    }

    void MemoryDoc::undo()
    {
        if (undo_.empty() || depth_ != 0)
            return;
        redo_.push_back(std::move(now_));
        now_ = std::move(undo_.back());
        undo_.pop_back();
        changed();
    }

    void MemoryDoc::redo()
    {
        if (redo_.empty() || depth_ != 0)
            return;
        undo_.push_back(std::move(now_));
        now_ = std::move(redo_.back());
        redo_.pop_back();
        changed();
    }
}
