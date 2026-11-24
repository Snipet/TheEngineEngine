// ui/kit/bind/host/BuildDoc.cpp: see BuildDoc.h.
#include "kit/bind/host/BuildDoc.h"

#include <algorithm>
#include <cmath>

namespace tee::ui
{
    namespace
    {
        constexpr std::size_t kPoints = 16;              // TABLE_POINTS (sim/spec/limits.fm)
        constexpr double kRpm = 3.14159265358979323846 / 30.0;

        std::string withoutSpec(const std::string& path)
        {
            return path.rfind("spec.", 0) == 0 ? path.substr(5) : path;
        }
    }

    bool axisRange(Quantity q, double& lo, double& hi) noexcept
    {
        switch (q)
        {
            case Quantity::rotation: lo = 0.0; hi = 10000.0 * kRpm; return true;
            case Quantity::pressure: lo = 0.0; hi = 400e3; return true;
            case Quantity::fraction: lo = 0.0; hi = 1.0; return true;
            case Quantity::temperature: lo = 233.15; hi = 423.15; return true;
            case Quantity::angle: lo = -360.0; hi = 720.0; return true;
            case Quantity::speed: lo = 0.0; hi = 100.0; return true;
            case Quantity::time: lo = 0.0; hi = 10.0; return true;
            default: return false;
        }
    }

    BuildDoc::BuildDoc(tee::Build& build) : build_(build)
    {
        const tee::Fields& fields = build.fields();
        const auto& all = fields.all();
        for (std::size_t k = 0; k < all.size(); ++k)
        {
            const tee::FieldInfo& f = all[k];
            if (f.block != "spec" || f.kind == tee::FieldKind::Rev)
                continue;
            FieldInfo info;
            info.path = withoutSpec(f.path);
            info.min = f.minimum.value_or(0.0);
            info.max = f.maximum.value_or(f.minimum.has_value() ? *f.minimum + 1.0 : 1.0);
            info.def = f.fallback();
            info.label = f.label;
            info.quantity = quantityOf(f.quantity);
            info.unit = f.unit;
            info.choices = f.choices;
            info.logTaper = f.taper == "log";
            info.decimals = f.decimals;
            info.group = f.group;
            info.help = f.help;
            info.hidden = f.hidden;
            switch (f.kind)
            {
                case tee::FieldKind::Number: info.kind = FieldKind::number; break;
                case tee::FieldKind::Integer: info.kind = FieldKind::integer; break;
                case tee::FieldKind::Switch: info.kind = FieldKind::toggle; info.min = 0.0; info.max = 1.0; break;
                case tee::FieldKind::Choice:
                    info.kind = FieldKind::choice;
                    if (!f.hasRange())
                        info.max = info.min + static_cast<double>(std::max<std::size_t>(f.choices.size(), 1) - 1);
                    break;
                case tee::FieldKind::Curve:
                case tee::FieldKind::Map:
                {
                    info.kind = f.kind == tee::FieldKind::Curve ? FieldKind::curve : FieldKind::map;
                    info.axes = { quantityOf(f.axisX), quantityOf(f.axisY) };
                    const std::vector<double>& v = build.values(static_cast<int>(k));
                    for (std::size_t a = 0; a < 2; ++a)
                    {
                        double lo = 0.0, hi = 1.0;
                        if (!axisRange(info.axes[a], lo, hi) && v.size() >= (a + 1) * kPoints)
                        {
                            // Its points' own span, and half again.
                            const auto first = v.begin() + static_cast<long>(a * kPoints);
                            const auto [mn, mx] = std::minmax_element(first, first + static_cast<long>(kPoints));
                            lo = std::min(0.0, *mn);
                            hi = std::max(1.0, *mx * 1.5);
                        }
                        info.axisLo[a] = lo;
                        info.axisHi[a] = hi;
                    }
                    if (info.kind == FieldKind::curve)
                    {
                        // A curve's y is its own quantity, the second of its axes; its range the Spec's, else that
                        // quantity's usual one.
                        if (info.quantity == Quantity::none)
                            info.quantity = info.axes[1];
                        if (!f.hasRange())
                            axisRange(info.quantity, info.min, info.max);
                    }
                    else if (!f.hasRange())
                        axisRange(info.quantity, info.min, info.max);
                    break;
                }
                case tee::FieldKind::Array:
                {
                    // Each element a number of its own.
                    info.kind = f.scalar == tee::Scalar::Bool ? FieldKind::toggle
                              : f.scalar == tee::Scalar::Int ? (f.choices.empty() ? FieldKind::integer : FieldKind::choice)
                                                             : FieldKind::number;
                    const std::string label = info.label.empty() ? info.path : info.label;
                    for (std::size_t e = 0; e < f.count; ++e)
                    {
                        FieldInfo el = info;
                        el.path = info.path + "[" + std::to_string(e) + "]";
                        el.label = label + " " + std::to_string(e + 1);
                        el.array = info.path;
                        el.arrayLabel = label;
                        el.element = static_cast<int>(e);
                        el.extent = static_cast<int>(f.count);
                        infos_.push_back(std::move(el));
                        sources_.push_back({ f.path, static_cast<int>(k), e });
                    }
                    continue;
                }
                case tee::FieldKind::Rev: continue;
            }
            infos_.push_back(std::move(info));
            sources_.push_back({ f.path, static_cast<int>(k), 0 });
        }
    }

    double BuildDoc::get(int i) const
    {
        const Source& s = sources_[static_cast<std::size_t>(i)];
        return build_.get(s.path, s.index);
    }

    void BuildDoc::set(int i, double si)
    {
        const Source& s = sources_[static_cast<std::size_t>(i)];
        build_.set(s.path, clampTo(infos_[static_cast<std::size_t>(i)], si), s.index);
    }

    // A curve's values: x[16], y[16], n.
    Curve16 BuildDoc::curve(int i) const
    {
        Curve16 c;
        const std::vector<double>& v = build_.values(sources_[static_cast<std::size_t>(i)].field);
        if (v.size() < 2 * kPoints + 1)
            return c;
        c.n = std::clamp(static_cast<int>(std::lround(v[2 * kPoints])), 0, kTableMax);
        for (std::size_t p = 0; p < kPoints; ++p)
        {
            c.x[p] = v[p];
            c.y[p] = v[kPoints + p];
        }
        return c;
    }

    void BuildDoc::setCurve(int i, const Curve16& c)
    {
        const auto n = static_cast<std::size_t>(std::clamp(c.n, 0, kTableMax));
        build_.setCurve(sources_[static_cast<std::size_t>(i)].path, std::vector<double>(c.x.begin(), c.x.begin() + static_cast<long>(n)),
                        std::vector<double>(c.y.begin(), c.y.begin() + static_cast<long>(n)));
    }

    // A map's values: x[16], y[16], z[16][16] (z[i][j] at x[i], y[j]), nx, ny.
    Map16 BuildDoc::map(int i) const
    {
        Map16 m;
        const std::vector<double>& v = build_.values(sources_[static_cast<std::size_t>(i)].field);
        const std::size_t zAt = 2 * kPoints, nAt = zAt + kPoints * kPoints;
        if (v.size() < nAt + 2)
            return m;
        m.nx = std::clamp(static_cast<int>(std::lround(v[nAt])), 0, kTableMax);
        m.ny = std::clamp(static_cast<int>(std::lround(v[nAt + 1])), 0, kTableMax);
        for (std::size_t p = 0; p < kPoints; ++p)
        {
            m.x[p] = v[p];
            m.y[p] = v[kPoints + p];
        }
        for (std::size_t xi = 0; xi < kPoints; ++xi)
            for (std::size_t yj = 0; yj < kPoints; ++yj)
                m.z[yj][xi] = v[zAt + xi * kPoints + yj];
        return m;
    }

    void BuildDoc::setMap(int i, const Map16& m)
    {
        const auto nx = static_cast<std::size_t>(std::clamp(m.nx, 0, kTableMax));
        const auto ny = static_cast<std::size_t>(std::clamp(m.ny, 0, kTableMax));
        std::vector<std::vector<double>> z(nx, std::vector<double>(ny, 0.0));
        for (std::size_t xi = 0; xi < nx; ++xi)
            for (std::size_t yj = 0; yj < ny; ++yj)
                z[xi][yj] = m.z[yj][xi];
        build_.setMap(sources_[static_cast<std::size_t>(i)].path, std::vector<double>(m.x.begin(), m.x.begin() + static_cast<long>(nx)),
                      std::vector<double>(m.y.begin(), m.y.begin() + static_cast<long>(ny)), z);
    }

    FrameServices buildServices(tee::Build& build, std::filesystem::path shipped, std::filesystem::path user, AppServices app)
    {
        FrameServices s;
        s.app = std::move(app);
        s.buildName = [&build] { return build.name.empty() ? std::string("Untitled") : build.name; };
        s.rename = [&build](const std::string& n) {
            build.name = n;
            return true;
        };
        s.undo = [&build] { build.undo(); };
        s.redo = [&build] { build.redo(); };
        const auto list = [shipped, user] { return tee::Presets::list(shipped, user); };
        s.builds = [list] {
            std::vector<std::string> out;
            for (const tee::PresetEntry& e : list())
                out.push_back(e.name + (e.user ? "" : " (shipped)"));
            return out;
        };
        s.load = [&build, list](const std::string& name, std::string& error) {
            for (const tee::PresetEntry& e : list())
                if (e.name + (e.user ? "" : " (shipped)") == name)
                {
                    std::vector<std::string> warnings;
                    return build.loadFile(e.file, tee::BuildPart::Whole, warnings, error);
                }
            error = "no build named " + name;
            return false;
        };
        s.save = [&build, user](const std::string& name, std::string& error) {
            if (name.empty() || name.find_first_of("/\\:") != std::string::npos)
            {
                error = "a build's name may not be empty or hold / \\ :";
                return false;
            }
            std::error_code ec;
            std::filesystem::create_directories(user, ec);
            return build.saveFile(user / (name + ".json"), tee::BuildPart::Whole, error);
        };
        return s;
    }
}
