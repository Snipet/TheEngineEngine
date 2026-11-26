// ui/kit/bind/Binding.cpp: see Binding.h.
#include "kit/bind/Binding.h"

#include "kit/gui/GuiSpec.h"
#include "kit/widgets/Basic.h"
#include "kit/widgets/Gauge.h"
#include "kit/widgets/Tables.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <deque>

namespace tee::ui
{
    namespace
    {
        bool logFits(const FieldInfo& f) noexcept { return f.logTaper && f.min > 0.0 && f.max > f.min; }

        bool sameText(std::string_view a, std::string_view b)
        {
            return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                       return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
                   });
        }

        std::string trimmed(std::string_view s)
        {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
                s.remove_prefix(1);
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
                s.remove_suffix(1);
            return std::string(s);
        }

        // Text for a .gui "text": no quote or newline in it.
        std::string quoted(std::string s)
        {
            for (char& c : s)
                if (c == '"' || c == '\n' || c == '\r')
                    c = '\'';
            return "\"" + s + "\"";
        }

        std::string nameOfPath(std::string_view path)
        {
            const std::size_t dot = path.rfind('.');
            return std::string(dot == std::string_view::npos ? path : path.substr(dot + 1));
        }

        std::string unitName(const Units& units, Quantity q, const std::string& fixed)
        {
            return q != Quantity::none ? units.unit(q).name : fixed;
        }

        // ---- the tables as the widgets' sources ----
        struct FieldCurveSource final : CurveSource
        {
            FieldDoc& doc;
            int field;
            FieldCurveSource(FieldDoc& d, int f) : doc(d), field(f) {}
            Curve16 curve() const override { return doc.curve(field); }
            void setCurve(const Curve16& c) override { doc.setCurve(field, c); }
            void beginEdit() override { doc.beginGesture(); }
            void endEdit() override { doc.endGesture(); }
        };

        struct FieldMapSource final : MapSource
        {
            FieldDoc& doc;
            int field;
            FieldMapSource(FieldDoc& d, int f) : doc(d), field(f) {}
            Map16 map() const override { return doc.map(field); }
            void setMap(const Map16& m) override { doc.setMap(field, m); }
            void beginEdit() override { doc.beginGesture(); }
            void endEdit() override { doc.endGesture(); }
        };

        // An array's elements as a map of one row: x the elements' numbers (1, 2, ...; not editable), z their values.
        struct ArraySource final : MapSource
        {
            FieldDoc& doc;
            std::vector<int> elements;
            ArraySource(FieldDoc& d, std::vector<int> e) : doc(d), elements(std::move(e)) {}
            Map16 map() const override
            {
                Map16 m;
                m.nx = static_cast<int>(std::min<std::size_t>(elements.size(), kTableMax));
                m.ny = 1;
                for (int i = 0; i < m.nx; ++i)
                {
                    m.x[static_cast<std::size_t>(i)] = i + 1;
                    m.z[0][static_cast<std::size_t>(i)] = doc.get(elements[static_cast<std::size_t>(i)]);
                }
                return m;
            }
            void setMap(const Map16& m) override
            {
                for (int i = 0; i < m.nx && i < static_cast<int>(elements.size()); ++i)
                {
                    const int f = elements[static_cast<std::size_t>(i)];
                    if (doc.get(f) != m.z[0][static_cast<std::size_t>(i)])
                        doc.set(f, m.z[0][static_cast<std::size_t>(i)]);
                }
            }
            void beginEdit() override { doc.beginGesture(); }
            void endEdit() override { doc.endGesture(); }
        };

        struct ArrayHolder
        {
            ArraySource source;
        };

        class FieldArrayEdit final : private ArrayHolder, public MapEdit
        {
        public:
            FieldArrayEdit(uint32_t id, std::string title, FieldDoc& doc, std::vector<int> elements, MapEdit::Axis x,
                           MapEdit::Axis y, MapEdit::Axis z)
                : ArrayHolder{ ArraySource(doc, std::move(elements)) },
                  MapEdit(id, std::move(title), source, std::move(x), std::move(y), std::move(z))
            {
            }
        };

        struct CurveHolder
        {
            FieldCurveSource source;
        };
        struct MapHolder
        {
            FieldMapSource source;
        };

        class FieldCurveEdit final : private CurveHolder, public CurveEdit
        {
        public:
            FieldCurveEdit(uint32_t id, std::string title, FieldDoc& doc, int field, PlotAxis x, PlotAxis y)
                : CurveHolder{ FieldCurveSource(doc, field) }, CurveEdit(id, std::move(title), source, std::move(x), std::move(y))
            {
            }
        };

        class FieldMapEdit final : private MapHolder, public MapEdit
        {
        public:
            FieldMapEdit(uint32_t id, std::string title, FieldDoc& doc, int field, MapEdit::Axis x, MapEdit::Axis y, MapEdit::Axis z)
                : MapHolder{ FieldMapSource(doc, field) }, MapEdit(id, std::move(title), source, std::move(x), std::move(y), std::move(z))
            {
            }
        };

        // A telemetry value against time: its last kSeconds, sampled each tick.
        class HistoryPlot final : public Plot
        {
        public:
            static constexpr double kSeconds = 10.0;
            HistoryPlot(uint32_t id, std::string title, TelemetrySource& telemetry, int handle, Quantity q, const Units& units)
                : Plot(id, std::move(title)), telemetry_(telemetry), handle_(handle), q_(q), units_(units)
            {
                series_.push_back({});
                series_[0].area = true;
                x_.lo = -kSeconds;
                x_.hi = 0.0;
                x_.label = "s";
            }

        protected:
            void step(float dt) override
            {
                const double v = units_.toUser(q_, telemetry_.value(handle_));
                for (double& t : times_)
                    t -= static_cast<double>(std::max(dt, 0.0f));
                times_.push_back(0.0);
                values_.push_back(v);
                while (!times_.empty() && times_.front() < -kSeconds)
                {
                    times_.pop_front();
                    values_.pop_front();
                }
                double lo = 0.0, hi = 1.0;
                for (const double s : values_)
                {
                    lo = std::min(lo, s);
                    hi = std::max(hi, s * 1.1);
                }
                const bool whole = vx0_ == x_.lo && vx1_ == x_.hi && vy0_ == y_.lo && vy1_ == y_.hi;
                y_.lo = lo;
                y_.hi = hi;
                if (whole)
                    resetView();
                series_[0].x.assign(times_.begin(), times_.end());
                series_[0].y.assign(values_.begin(), values_.end());
                touched();
            }
            bool still() const override { return false; }   // it moves with time
            const char* kind() const override { return "history"; }

        private:
            TelemetrySource& telemetry_;
            int handle_;
            Quantity q_;
            const Units& units_;
            std::deque<double> times_, values_;
        };

        TableText textOf(const Units& units, Quantity q, int decimals)
        {
            TableText t;
            t.format = [&units, q, decimals](double si) { return units.number(q, si, decimals); };
            t.parse = [&units, q](std::string_view s, double& si) { return units.parse(q, s, si); };
            return t;
        }

        // The telemetry value an axis of a map most likely is, for its live point (the engine's speed, its load).
        std::string_view liveName(Quantity q)
        {
            switch (q)
            {
                case Quantity::rotation: return telemetry::kSpeed;
                case Quantity::pressure: return telemetry::kManifold;
                case Quantity::fraction: return telemetry::kThrottle;
                case Quantity::speed: return telemetry::kCarSpeed;
                case Quantity::temperature: return telemetry::kCoolant;
                default: return {};
            }
        }
    }

    // ---- values ------------------------------------------------------------------------------------------------------
    float norm01(const FieldInfo& f, double si) noexcept
    {
        if (f.kind == FieldKind::toggle)
            return si >= 0.5 ? 1.0f : 0.0f;
        if (!(f.max > f.min))
            return 0.0f;
        if (f.kind == FieldKind::number && logFits(f))
            return si > f.min ? static_cast<float>(std::clamp(std::log(si / f.min) / std::log(f.max / f.min), 0.0, 1.0)) : 0.0f;
        return static_cast<float>(std::clamp((si - f.min) / (f.max - f.min), 0.0, 1.0));
    }

    double plainOf01(const FieldInfo& f, float v01) noexcept
    {
        const double t = std::clamp(static_cast<double>(v01), 0.0, 1.0);
        if (f.kind == FieldKind::number && logFits(f))
            return std::clamp(f.min * std::exp(t * std::log(f.max / f.min)), f.min, f.max);
        return clampTo(f, f.min + t * (f.max - f.min));
    }

    std::string fieldText(const FieldInfo& f, double si, const Units& units)
    {
        switch (f.kind)
        {
            case FieldKind::toggle: return si >= 0.5 ? "ON" : "OFF";
            case FieldKind::choice:
            {
                const long k = std::lround(si - f.min);
                if (k >= 0 && static_cast<std::size_t>(k) < f.choices.size())
                    return f.choices[static_cast<std::size_t>(k)];
                return formatNumber(si, 0);
            }
            case FieldKind::integer:
                return formatNumber(si, 0) + (f.unit.empty() ? "" : " " + f.unit);
            case FieldKind::number:
            case FieldKind::curve:
            case FieldKind::map:
                break;
        }
        if (f.quantity != Quantity::none)
            return units.format(f.quantity, si, f.decimals, f.preferUnit);
        return formatNumber(si, f.decimals) + (f.unit.empty() ? "" : " " + f.unit);
    }

    bool parseField(const FieldInfo& f, std::string_view text, const Units& units, double& si)
    {
        const std::string t = trimmed(text);
        if (t.empty())
            return false;
        if (f.kind == FieldKind::toggle)
        {
            for (const char* on : { "on", "1", "true", "yes" })
                if (sameText(t, on))
                    return si = 1.0, true;
            for (const char* off : { "off", "0", "false", "no" })
                if (sameText(t, off))
                    return si = 0.0, true;
            return false;
        }
        if (f.kind == FieldKind::choice)
            for (std::size_t k = 0; k < f.choices.size(); ++k)
                if (sameText(t, f.choices[k]))
                    return si = f.min + static_cast<double>(k), true;
        if (f.kind == FieldKind::number && f.quantity != Quantity::none)
        {
            if (!units.parse(f.quantity, t, si, f.preferUnit))
                return false;
            si = clampTo(f, si);
            return true;
        }
        char* end = nullptr;
        const double v = std::strtod(t.c_str(), &end);
        if (end == t.c_str() || !std::isfinite(v))
            return false;
        const std::string rest = trimmed(end);
        if (!rest.empty() && !sameText(rest, f.unit))
            return false;
        si = clampTo(f, v);
        return true;
    }

    // ---- FieldPort ---------------------------------------------------------------------------------------------------
    FieldPort::FieldPort(FieldDoc& doc, int field, const Units& units)
        : doc_(doc), field_(field), units_(units), path_(doc.info(field).path)
    {
    }

    float FieldPort::value01() const { return norm01(info(), doc_.get(field_)); }
    float FieldPort::default01() const { return norm01(info(), info().def); }

    int FieldPort::numSteps() const
    {
        const FieldInfo& f = info();
        switch (f.kind)
        {
            case FieldKind::toggle: return 1;
            case FieldKind::integer:
            case FieldKind::choice: return std::max(1, static_cast<int>(std::lround(f.max - f.min)));
            default: return 0;
        }
    }

    void FieldPort::beginGesture()
    {
        ++open_;
        doc_.beginGesture();
    }

    void FieldPort::setValue01(float v) { doc_.set(field_, plainOf01(info(), v)); }

    void FieldPort::endGesture()
    {
        if (open_ > 0)
            --open_;
        doc_.endGesture();
    }

    std::string FieldPort::text(float v01) const { return fieldText(info(), plainOf01(info(), v01), units_); }

    // ---- FieldParams -------------------------------------------------------------------------------------------------
    FieldParams::FieldParams(FieldDoc& doc, const Units& units, std::vector<int> fields) : doc_(doc), units_(units)
    {
        for (const int i : fields)
            if (i >= 0 && i < doc.count() && !doc.info(i).table())
                fields_.push_back(i);
        refresh();
    }

    void FieldParams::refresh()
    {
        infos_.clear();
        for (const int i : fields_)
        {
            const FieldInfo& f = doc_.info(i);
            ParamInfo p;
            p.id = f.path;
            p.label = f.label.empty() ? nameOfPath(f.path) : f.label;
            p.group = f.group;
            p.min = f.min;
            p.max = f.max;
            p.def = f.def;
            switch (f.kind)
            {
                case FieldKind::toggle: p.kind = ParamInfo::Kind::toggle; break;
                case FieldKind::choice:
                    p.kind = ParamInfo::Kind::choice;
                    p.choices = f.choices;
                    break;
                case FieldKind::integer:
                    p.kind = ParamInfo::Kind::integer;
                    p.unit = f.unit;
                    p.decimals = 0;
                    break;
                default:
                {
                    p.kind = ParamInfo::Kind::real;
                    const Quantity q = f.quantity;
                    if (q != Quantity::none)
                    {
                        p.min = units_.toUser(q, f.min, f.preferUnit);
                        p.max = units_.toUser(q, f.max, f.preferUnit);
                        p.def = units_.toUser(q, f.def, f.preferUnit);
                    }
                    p.unit = q != Quantity::none ? units_.unit(q, f.preferUnit).name : f.unit;
                    p.decimals = f.decimals >= 0 ? f.decimals : (q != Quantity::none ? units_.unit(q, f.preferUnit).decimals : -1);
                    p.logTaper = f.logTaper && p.min > 0.0 && p.max > p.min;
                    break;
                }
            }
            infos_.push_back(std::move(p));
        }
    }

    double FieldParams::get(int i) const
    {
        const FieldInfo& f = doc_.info(field(i));
        const double v = doc_.get(field(i));
        return f.kind == FieldKind::number && f.quantity != Quantity::none ? units_.toUser(f.quantity, v, f.preferUnit) : v;
    }

    void FieldParams::set(int i, double value)
    {
        const FieldInfo& f = doc_.info(field(i));
        doc_.set(field(i), f.kind == FieldKind::number && f.quantity != Quantity::none ? units_.toSi(f.quantity, value, f.preferUnit)
                                                                                       : value);
    }

    int FieldParams::findParam(std::string_view id) const
    {
        for (std::size_t k = 0; k < fields_.size(); ++k)
            if (infos_[k].id == id)
                return static_cast<int>(k);
        return -1;
    }

    // ---- generated pages ---------------------------------------------------------------------------------------------
    std::vector<int> sectionFields(const FieldDoc& doc, std::string_view section)
    {
        std::vector<int> out;
        for (int i = 0; i < doc.count(); ++i)
            if (!doc.info(i).hidden && doc.info(i).section() == section)
                out.push_back(i);
        return out;
    }

    bool arrayAsTable(const FieldInfo& f) noexcept
    {
        return !f.array.empty() && f.shape[0] == 0 && f.extent > kArrayKnobs && f.extent <= kTableMax;
    }

    std::vector<std::vector<int>> splitPages(const FieldDoc& doc, const std::vector<int>& fields, int maxControls)
    {
        // A table, and an array's element shown in its array's table, is no control of the panel's.
        const auto controlOf = [&](int f) { return doc.info(f).table() || arrayAsTable(doc.info(f)) ? 0 : 1; };
        std::vector<std::vector<int>> pages(1);
        int controls = 0;
        std::size_t i = 0;
        while (i < fields.size())
        {
            // A run of one group.
            std::size_t j = i;
            int run = 0;
            while (j < fields.size() && doc.info(fields[j]).group == doc.info(fields[i]).group)
                run += controlOf(fields[j++]);
            if (controls > 0 && controls + run > maxControls)
            {
                pages.emplace_back();
                controls = 0;
            }
            for (; i < j; ++i)
            {
                const int c = controlOf(fields[i]);
                if (controls + c > maxControls)
                {
                    pages.emplace_back();
                    controls = 0;
                }
                pages.back().push_back(fields[i]);
                controls += c;
            }
        }
        if (pages.back().empty() && pages.size() > 1)
            pages.pop_back();
        return pages;
    }

    std::string pageGui(const FieldDoc& doc, const std::vector<int>& fields, std::string_view title)
    {
        std::string out = "column gap=12 pad=4 {";
        if (fields.empty())
            return out + " label \"NOTHING TO SET HERE YET\" size=small }";
        std::size_t i = 0;
        while (i < fields.size())
        {
            const std::string& group = doc.info(fields[i]).group;
            std::size_t j = i;
            while (j < fields.size() && doc.info(fields[j]).group == group)
                ++j;
            std::string heading = group.empty() ? std::string(title.empty() ? doc.info(fields[i]).section() : title) : group;
            out += " section " + quoted(upper(heading)) + " layout=column gap=10 {";
            std::string row, tables;
            for (std::size_t k = i; k < j; ++k)
            {
                const FieldInfo& f = doc.info(fields[k]);
                if (arrayAsTable(f))
                {
                    // An array of 9 to 16 elements: one row of cells (`array`), at its first element on the page.
                    const bool first = std::none_of(fields.begin(), fields.begin() + static_cast<long>(k), [&](int o) {
                        return doc.info(o).array == f.array;
                    });
                    if (first)
                        tables += " array " + f.array + " label=" + quoted(f.arrayLabel.empty() ? nameOfPath(f.array) : f.arrayLabel) + " ;";
                    continue;
                }
                switch (f.kind)
                {
                    case FieldKind::number:
                    case FieldKind::integer: row += " knob " + f.path + " ;"; break;
                    case FieldKind::choice: row += " menu " + f.path + " ;"; break;
                    case FieldKind::toggle: row += " switch " + f.path + " \"OFF\" \"ON\" ;"; break;
                    case FieldKind::curve:
                        tables += " curve " + f.path + " label=" + quoted(f.label.empty() ? nameOfPath(f.path) : f.label) + " ;";
                        break;
                    case FieldKind::map:
                        tables += " map " + f.path + " label=" + quoted(f.label.empty() ? nameOfPath(f.path) : f.label) + " ;";
                        break;
                }
            }
            if (!row.empty())
                out += " row wrap gap=12 {" + row + " } ;";
            out += tables + " } ;";
            i = j;
        }
        return out + " }";
    }

    std::vector<int> fieldsOfGui(const FieldDoc& doc, std::string_view gui)
    {
        std::vector<bool> want(static_cast<std::size_t>(doc.count()), false);
        const auto nameStart = [](char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; };
        const auto nameChar = [&](char c) { return nameStart(c) || std::isdigit(static_cast<unsigned char>(c)); };
        for (std::size_t i = 0; i < gui.size();)
        {
            if (gui.substr(i).starts_with("auto") && (i == 0 || !nameChar(gui[i - 1])) && i + 4 < gui.size() && !nameChar(gui[i + 4]))
            {
                const std::size_t q0 = gui.find('"', i), nl = gui.find('\n', i);
                const std::size_t q1 = q0 == std::string_view::npos ? q0 : gui.find('"', q0 + 1);
                if (q1 != std::string_view::npos && (nl == std::string_view::npos || q1 < nl))
                {
                    for (const int f : sectionFields(doc, gui.substr(q0 + 1, q1 - q0 - 1)))
                        want[static_cast<std::size_t>(f)] = true;
                    i = q1 + 1;
                    continue;
                }
            }
            if (!nameStart(gui[i]) || (i > 0 && nameChar(gui[i - 1])))
            {
                ++i;
                continue;
            }
            std::size_t k = i;
            while (k < gui.size() && (nameChar(gui[k]) || (gui[k] == '.' && k + 1 < gui.size() && nameStart(gui[k + 1]))))
                ++k;
            // U1: an array's element ("engine.bankAngle[2]"), as the lexer reads it: digits in brackets.
            while (k + 2 < gui.size() && gui[k] == '[' && std::isdigit(static_cast<unsigned char>(gui[k + 1])))
            {
                std::size_t e = k + 1;
                while (e < gui.size() && std::isdigit(static_cast<unsigned char>(gui[e])))
                    ++e;
                if (e >= gui.size() || gui[e] != ']')
                    break;
                k = e + 1;
            }
            if (const int f = doc.find(gui.substr(i, k - i)); f >= 0)
                want[static_cast<std::size_t>(f)] = true;
            i = k;
        }
        std::vector<int> out;
        for (int pass = 0; pass < 2; ++pass)
            for (int f = 0; f < doc.count(); ++f)
                if (want[static_cast<std::size_t>(f)] && doc.info(f).table() == (pass == 1))
                    out.push_back(f);
        return out;
    }

    // ---- the widget elements -----------------------------------------------------------------------------------------
    TelemetryLook telemetryLook(std::string_view name)
    {
        struct Row
        {
            std::string_view name;
            TelemetryLook look;
        };
        static const Row rows[] = {
            { telemetry::kSpeed, { Quantity::rotation, 0.0, 890.0, "Engine" } },        // 0 to 8500 rpm
            { telemetry::kRedline, { Quantity::rotation, 0.0, 890.0, "Redline" } },
            { telemetry::kThrottle, { Quantity::fraction, 0.0, 1.0, "Throttle" } },
            { telemetry::kManifold, { Quantity::pressure, 0.0, 250e3, "Manifold" } },
            { telemetry::kCoolant, { Quantity::temperature, 233.15, 413.15, "Coolant" } },
            { telemetry::kOilPressure, { Quantity::pressure, 0.0, 700e3, "Oil" } },
            { telemetry::kCarSpeed, { Quantity::speed, 0.0, 300.0 / 3.6, "Speed" } },
            { telemetry::kGear, { Quantity::none, -1.0, 8.0, "Gear" } },
            { telemetry::kBoost, { Quantity::pressure, -100e3, 250e3, "Boost" } },
            { telemetry::kCarBrake, { Quantity::fraction, 0.0, 1.0, "Brake" } },
            { telemetry::kCarClutch, { Quantity::fraction, 0.0, 1.0, "Clutch" } },
            { telemetry::kState, { Quantity::none, 0.0, 3.0, "State" } },
        };
        for (const Row& r : rows)
            if (r.name == name)
                return r.look;
        return { Quantity::none, 0.0, 1.0, nameOfPath(name) };
    }

    std::unique_ptr<Item> fieldWidget(const GuiNode& node, uint32_t id, FieldDoc& doc, const Units& units,
                                      TelemetrySource& telemetry)
    {
        const std::string& w = node.widget;
        if (w == "array")
        {
            const std::vector<int> elements = arrayElements(doc, node.file);
            if (elements.empty())
                return nullptr;
            const FieldInfo& first = doc.info(elements.front());
            const std::string title = !node.text.empty() ? node.text : (first.arrayLabel.empty() ? nameOfPath(node.file) : first.arrayLabel);
            const std::string unit = unitName(units, first.quantity, first.unit);
            TableText none{ [](double v) { return formatNumber(v, 0); }, [](std::string_view, double&) { return false; } };
            TableText row{ [](double) { return std::string(); }, [](std::string_view, double&) { return false; } };
            // A value the document refuses (FieldDoc::accepts: the host's least stated bore, one rule for every element)
            // is refused as text that is no number: the cell keeps the typing, marked.
            TableText values{ [&units, &first](double v) { return fieldText(first, v, units); },
                              [&units, &first, &doc, e = elements.front()](std::string_view s, double& v) {
                                  return parseField(first, s, units, v) && doc.accepts(e, v);
                              } };
            // A unit's name is in the title, not in each cell: the cells show numbers (a choice its name).
            if (first.kind == FieldKind::number && first.quantity != Quantity::none)
                values.format = [&units, q = first.quantity, d = first.decimals](double v) { return units.number(q, v, d); };
            // The sentence the table shows on its title's line while a refused typing is kept, worded as Rows' are.
            values.refusal = [&units, &first, &doc, e = elements.front(), unit](std::string_view s) {
                double v = 0.0;
                if (!parseField(first, s, units, v) && first.kind == FieldKind::choice)
                {
                    std::string names;
                    for (std::size_t k = 0; k < first.choices.size() && k < 6; ++k)
                        names += (k > 0 ? ", " : "") + first.choices[k];
                    return "One of: " + names + (first.choices.size() > 6 ? ", ..." : "");
                }
                if (!parseField(first, s, units, v))
                {
                    const bool q = first.kind == FieldKind::number && first.quantity != Quantity::none;
                    const std::string lo = q ? units.number(first.quantity, first.min, first.decimals) : fieldText(first, first.min, units);
                    const std::string hi = q ? units.number(first.quantity, first.max, first.decimals) : fieldText(first, first.max, units);
                    return "Not a number" + (unit.empty() ? std::string() : " of " + unit) + ": " + lo + " to " + hi
                         + (unit.empty() || !q ? std::string() : " " + unit);
                }
                std::string why;
                if (!doc.accepts(e, v, &why))
                    return "Out of range: " + why;
                return std::string();
            };
            MapEdit::Axis x{ "#", 1.0, static_cast<double>(elements.size()), std::move(none) };
            MapEdit::Axis y{ unit.empty() ? std::string("value") : unit, 0.0, 0.0, std::move(row) };
            MapEdit::Axis z{ unit.empty() ? std::string("value") : unit, first.min, first.max, std::move(values) };
            return std::make_unique<FieldArrayEdit>(id, title, doc, elements, std::move(x), std::move(y), std::move(z));
        }
        if (w == "map" || w == "curve")
        {
            const int f = doc.find(node.file);
            if (f < 0)
                return nullptr;
            const FieldInfo& info = doc.info(f);
            const std::string title = !node.text.empty() ? node.text : (info.label.empty() ? nameOfPath(info.path) : info.label);
            const std::string zUnit = unitName(units, info.quantity, info.unit);
            if (w == "curve" && info.kind == FieldKind::curve)
            {
                PlotAxis x, y;
                x.lo = info.axisLo[0];
                x.hi = info.axisHi[0];
                x.label = unitName(units, info.axes[0], {});
                x.format = [&units, q = info.axes[0]](double si) { return units.number(q, si); };
                y.lo = info.min;
                y.hi = info.max;
                y.label = zUnit;
                y.format = [&units, q = info.quantity, d = info.decimals](double si) { return units.number(q, si, d); };
                // The plot's axes are in SI, as the curve; its numbers are in the user's units (the format).
                return std::make_unique<FieldCurveEdit>(id, title, doc, f, std::move(x), std::move(y));
            }
            if (w == "map" && info.kind == FieldKind::map)
            {
                MapEdit::Axis x{ unitName(units, info.axes[0], "x"), info.axisLo[0], info.axisHi[0], textOf(units, info.axes[0], -1) };
                MapEdit::Axis y{ unitName(units, info.axes[1], "y"), info.axisLo[1], info.axisHi[1], textOf(units, info.axes[1], -1) };
                MapEdit::Axis z{ zUnit.empty() ? std::string("value") : zUnit, info.min, info.max,
                                 textOf(units, info.quantity, info.decimals) };
                auto edit = std::make_unique<FieldMapEdit>(id, title, doc, f, std::move(x), std::move(y), std::move(z));
                const int hx = telemetry.findValue(liveName(info.axes[0])), hy = telemetry.findValue(liveName(info.axes[1]));
                const int state = telemetry.findValue(telemetry::kState);
                if (hx >= 0 && hy >= 0)
                    edit->setLive([&telemetry, hx, hy, state](double& lx, double& ly) {
                        if (state >= 0 && telemetry.value(state) < 0.5)
                            return false;                // the engine is off: no operating point
                        lx = telemetry.value(hx);
                        ly = telemetry.value(hy);
                        return true;
                    });
                return edit;
            }
            return nullptr;
        }
        if (w != "gauge" && w != "bar" && w != "lamp" && w != "plot")
            return nullptr;
        const int handle = telemetry.findValue(node.file);
        const TelemetryLook look = telemetryLook(node.file);
        const std::string title = !node.text.empty() ? node.text : look.label;
        const Quantity q = look.quantity;
        if (w == "gauge")
        {
            auto g = std::make_unique<Gauge>(id, title, [&telemetry, &units, handle, q] {
                return units.toUser(q, telemetry.value(handle));
            }, units.toUser(q, look.lo), units.toUser(q, look.hi));
            g->setUnit(units.unit(q).name);
            if (q == Quantity::rotation && std::string_view(units.unit(q).name) == "rpm")
            {
                g->setTicks(static_cast<int>(std::lround(units.toUser(q, look.hi) / 1000.0)), 2);
                g->setNumerals([](double v) { return formatNumber(v / 1000.0, 0); });
                g->setUnit("x1000 rpm");
                g->setReadout([](double v) { return formatNumber(v, 0); });
                const int red = telemetry.findValue(telemetry::kRedline);
                if (red >= 0 && telemetry.value(red) > 0.0)
                    g->setRedline(units.toUser(q, telemetry.value(red)));
            }
            else
            {
                // Divisions at round values: the first count from 5 to 10 whose step is 1, 2, 2.5 or 5 times a power of
                // ten (300 km/h: 6 of 50).
                const double span = units.toUser(q, look.hi) - units.toUser(q, look.lo);
                for (int n = 5; n <= 10; ++n)
                {
                    const double step = span / n, mag = std::pow(10.0, std::floor(std::log10(step)));
                    const double k = step / mag;
                    if (std::fabs(k - std::round(k * 2.0) / 2.0) < 1e-9 && (k == 1.0 || k == 2.0 || k == 2.5 || k == 5.0))
                    {
                        g->setTicks(n, 2);
                        break;
                    }
                }
                g->setReadout([&units, q](double v) { return formatNumber(v, units.unit(q).decimals); });
            }
            return g;
        }
        if (w == "bar")
            return std::make_unique<Bar>(id, title, [&telemetry, handle, look] {
                return static_cast<float>((telemetry.value(handle) - look.lo) / (look.hi - look.lo));
            }, [&telemetry, &units, handle, q] { return units.format(q, telemetry.value(handle)); });
        if (w == "lamp")
            return std::make_unique<Lamp>(id, title, [&telemetry, handle] { return telemetry.value(handle) > 0.5; });
        auto p = std::make_unique<HistoryPlot>(id, title, telemetry, handle, q, units);
        p->yAxis().label = units.unit(q).name;
        return p;
    }

    std::vector<int> fieldsOfWidget(const FieldDoc& doc, const GuiNode& node)
    {
        if (node.kind != GuiKind::widget)
            return {};
        if (node.widget == "array")
            return arrayElements(doc, node.file);
        if (node.widget == "map" || node.widget == "curve")
            if (const int f = doc.find(node.file); f >= 0)
                return { f };
        return {};
    }

    // ---- FieldPage ---------------------------------------------------------------------------------------------------
    std::vector<int> FieldPage::fieldsOf(const FieldDoc& doc, const Sources& files)
    {
        for (const SourceFile& f : files)
            if (isGuiFile(f.name))
                return fieldsOfGui(doc, f.text);
        return {};
    }

    GuiPanel::Options FieldPage::withBinding(Options o, FieldDoc& doc, const Units& units, TelemetrySource& telemetry,
                                             const FieldParams& params)
    {
        // U1: hover help: a parameter's field's ui::help (the FieldParams base is made before the GuiPanel base).
        if (!o.help)
            o.help = [&doc, &params](int i) {
                return i >= 0 && i < params.count() ? doc.info(params.field(i)).help : std::string();
            };
        if (!o.widget)
            o.widget = [&doc, &units, &telemetry](const GuiNode& n, uint32_t id) { return fieldWidget(n, id, doc, units, telemetry); };
        if (!o.autoPage)
            o.autoPage = [&doc](std::string_view section) {
                const std::vector<int> f = sectionFields(doc, section);
                return f.empty() ? std::string() : pageGui(doc, f);
            };
        return o;
    }

    FieldPage::FieldPage(FieldDoc& doc, const Units& units, TelemetrySource& telemetry, Sources files, Options options)
        : detail::FieldPageParams(doc, units, fieldsOf(doc, files)),
          GuiPanel(std::move(files), fieldParams, telemetry, withBinding(std::move(options), doc, units, telemetry, fieldParams))
    {
        // An array's element past its extent: the reader says there is no such parameter; say why.
        for (const SourceFile& f : GuiPanel::files())
        {
            if (!isGuiFile(f.name))
                continue;
            const std::string& t = f.text;
            int line = 1;
            std::size_t lineStart = 0;
            for (std::size_t i = 0; i < t.size(); ++i)
            {
                if (t[i] == '\n')
                {
                    ++line;
                    lineStart = i + 1;
                    continue;
                }
                if (t[i] != '[' || i == 0)
                    continue;
                std::size_t b = i;
                while (b > 0 && (std::isalnum(static_cast<unsigned char>(t[b - 1])) || t[b - 1] == '_' || t[b - 1] == '.'))
                    --b;
                // Its indexes: one, or two for an array of arrays.
                std::vector<long> index;
                std::size_t e = i;
                while (e < t.size() && t[e] == '[' && index.size() < 2)
                {
                    std::size_t d = e + 1;
                    while (d < t.size() && std::isdigit(static_cast<unsigned char>(t[d])))
                        ++d;
                    if (d == e + 1 || d >= t.size() || t[d] != ']')
                        break;
                    index.push_back(std::strtol(t.substr(e + 1, d - e - 1).c_str(), nullptr, 10));
                    e = d + 1;
                }
                if (index.empty() || b == i)
                    continue;
                const std::string base = t.substr(b, i - b);
                const std::vector<int> elements = arrayElements(doc, base);
                if (elements.empty())
                    continue;
                const std::array<int, 2> shape = doc.info(elements.front()).shape;
                const std::string at = f.name + ":" + std::to_string(line) + ":" + std::to_string(b - lineStart + 1) + ": note: ";
                const std::string named = t.substr(b, e - b);
                if (shape[0] > 0)
                {
                    if (index.size() != 2 || index[0] >= shape[0] || index[1] >= shape[1])
                        addNote(at + named + " is not an element: " + base + " has " + std::to_string(shape[0]) + " x "
                                + std::to_string(shape[1]) + " elements, " + base + "[0][0] to " + base + "["
                                + std::to_string(shape[0] - 1) + "][" + std::to_string(shape[1] - 1) + "]\n");
                }
                else if (index.size() != 1 || index[0] >= static_cast<long>(elements.size()))
                    addNote(at + named + " is past the array: it has " + std::to_string(elements.size()) + " elements, "
                            + base + "[0] to " + base + "[" + std::to_string(elements.size() - 1) + "]\n");
                i = e - 1;
            }
        }
    }
}
