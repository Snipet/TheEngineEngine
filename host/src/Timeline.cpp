// host/src/Timeline.cpp: scenarios read, played and checked (Timeline.h).
#include "tee/Timeline.h"

#include "tee/Measures.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>

namespace tee
{
    using nlohmann::json;

    void Timeline::sort()
    {
        std::stable_sort(events.begin(), events.end(),
                         [](const TimelineEvent& a, const TimelineEvent& b) { return a.time < b.time; });
    }

    std::string telemetryColumn(std::string_view name)
    {
        std::string out;
        for (const char c : name)
            if (c != ' ')
                out += c;
        if (out.rfind("tel.", 0) == 0)
            out.erase(0, 4);
        return out;
    }

    namespace
    {
        std::size_t editDistance(const std::string& a, const std::string& b)
        {
            std::vector<std::size_t> row(b.size() + 1);
            for (std::size_t j = 0; j <= b.size(); ++j)
                row[j] = j;
            for (std::size_t i = 1; i <= a.size(); ++i)
            {
                std::size_t diagonal = row[0];
                row[0] = i;
                for (std::size_t j = 1; j <= b.size(); ++j)
                {
                    const std::size_t up = row[j];
                    row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? 0 : 1)});
                    diagonal = up;
                }
            }
            return row[b.size()];
        }

        // The three telemetry fields nearest a name by edit distance, said: "nearest: engine.speed, ..."
        std::string nearestTelemetry(const Fields& fields, const std::string& name)
        {
            std::vector<std::pair<std::size_t, std::string>> all;
            for (const FieldInfo& f : fields.all())
                if (f.block == "tel" && !f.table() && f.scalar != Scalar::None)
                {
                    const std::string column = f.path.substr(4);
                    all.emplace_back(editDistance(name, column), column);
                }
            std::sort(all.begin(), all.end());
            std::string out;
            for (std::size_t k = 0; k < all.size() && k < 3; ++k)
                out += (k == 0 ? "nearest: " : ", ") + all[k].second;
            return out;
        }
    } // namespace

    bool telemetryElement(const Fields& fields, std::string_view name, const FieldInfo*& field, std::size_t& index,
                          std::string& error)
    {
        const std::string column = telemetryColumn(name);
        const std::size_t bracket = column.find('[');
        const std::string path = "tel." + column.substr(0, bracket);
        field = fields.find(path);
        index = 0;
        if (field == nullptr || field->block != "tel" || field->table() || field->scalar == Scalar::None)
        {
            error = "no telemetry field '" + column.substr(0, bracket) + "' ("
                    + nearestTelemetry(fields, column.substr(0, bracket)) + ")";
            field = nullptr;
            return false;
        }
        std::vector<std::size_t> at;
        for (std::size_t i = bracket; i != std::string::npos && i < column.size();)
        {
            const std::size_t close = column.find(']', i);
            if (column[i] != '[' || close == std::string::npos)
            {
                error = "'" + column + "': an index is written [n]";
                field = nullptr;
                return false;
            }
            const std::string digits = column.substr(i + 1, close - i - 1);
            if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos)
            {
                error = "'" + column + "': an index is a whole number";
                field = nullptr;
                return false;
            }
            at.push_back(static_cast<std::size_t>(std::stoul(digits)));
            i = close + 1;
        }
        const std::vector<std::size_t>& extents = field->extents;
        const std::string base = column.substr(0, bracket);
        if (extents.empty() && !at.empty())
        {
            error = "'" + column + "': " + base + " is no array";
            field = nullptr;
            return false;
        }
        if (!extents.empty() && at.size() != extents.size())
        {
            error = "'" + column + "': " + base + " is an array of " + std::to_string(field->count)
                    + " elements: name one, as " + base + "[0]" + (extents.size() > 1 ? "[0]" : "");
            field = nullptr;
            return false;
        }
        for (std::size_t d = 0; d < at.size(); ++d)
        {
            if (at[d] >= extents[d])
            {
                error = "'" + column + "': " + base + " has " + std::to_string(extents[d]) + " elements"
                        + (extents.size() > 1 ? " in dimension " + std::to_string(d + 1) : "") + ", 0 to "
                        + std::to_string(extents[d] - 1);
                field = nullptr;
                return false;
            }
            index = index * extents[d] + at[d];
        }
        return true;
    }

    int RunResult::column(std::string_view name) const noexcept
    {
        const std::string path = telemetryColumn(name);
        for (std::size_t c = 0; c < columns.size(); ++c)
            if (columns[c] == path)
                return static_cast<int>(c);
        return -1;
    }

    bool parseTimeline(std::string_view text, std::string_view fileName, Timeline& t, std::string& error)
    {
        const std::string where = std::string(fileName) + ": ";
        const json root = json::parse(text, nullptr, false);
        if (root.is_discarded() || !root.is_object())
        {
            error = where + "not JSON, or not an object";
            return false;
        }
        // "tee-scenario" (version 1), and K3a's "tee-engine-scenario/1", which is the same with "ctl" for "controls"
        const std::string format = root.value("format", std::string());
        if (format != "tee-scenario" && format != "tee-engine-scenario/1")
        {
            error = where + "its \"format\" is not \"tee-scenario\"";
            return false;
        }
        t = Timeline{};
        t.name = root.value("name", std::string(std::filesystem::path(std::string(fileName)).stem().generic_string()));
        t.description = root.value("description", std::string());
        t.build = root.value("build", std::string());
        t.rate = root.value("rate", 48000.0);
        t.block = root.value("block", 512);
        t.seconds = root.value("seconds", 0.0);
        t.warm = root.value("warm", false);
        if (!(t.rate > 0.0) || t.block < 1 || !(t.seconds >= 0.0))
        {
            error = where + "\"rate\", \"block\" and \"seconds\" are numbers above 0";
            return false;
        }
        if (root.contains("events"))
        {
            if (!root["events"].is_array())
            {
                error = where + "\"events\" is an array";
                return false;
            }
            int index = 0;
            for (const json& e : root["events"])
            {
                ++index;
                const std::string at = where + "event " + std::to_string(index) + ": ";
                if (!e.is_object() || !e.contains("t") || !e["t"].is_number())
                {
                    error = at + "an object with a time \"t\" in seconds";
                    return false;
                }
                const double time = e["t"].get<double>();
                const double ramp = e.value("ramp", 0.0);
                const char* controlsKey = e.contains("ctl") ? "ctl" : "controls";
                if (e.contains(controlsKey))
                {
                    if (!e[controlsKey].is_object())
                    {
                        error = at + "\"ctl\" is an object of numbers";
                        return false;
                    }
                    for (auto it = e[controlsKey].begin(); it != e[controlsKey].end(); ++it)
                    {
                        SimControls probe;
                        if (!it.value().is_number() || !setControl(probe, it.key(), 0.0))
                        {
                            error = at + "\"" + it.key() + "\" is no control (02-contracts.md, section 4), or not a number";
                            return false;
                        }
                        t.events.push_back({time, TimelineEvent::Kind::Control, it.key(), it.value().get<double>(), {}, ramp});
                    }
                }
                if (e.contains("press"))
                {
                    const json& p = e["press"];
                    const auto add = [&](const json& name) {
                        if (!name.is_string())
                            return false;
                        t.events.push_back({time, TimelineEvent::Kind::Press, name.get<std::string>(), 1.0, {}, 0.0});
                        return true;
                    };
                    bool ok = p.is_array() ? std::all_of(p.begin(), p.end(), add) : add(p);
                    if (!ok)
                    {
                        error = at + "\"press\" is a counter's name (shiftUp, shiftDown, strip, bakeMaps) or an array of them";
                        return false;
                    }
                }
                if (e.contains("spec"))
                {
                    if (!e["spec"].is_object())
                    {
                        error = at + "\"spec\" is an object of paths and values";
                        return false;
                    }
                    for (auto it = e["spec"].begin(); it != e["spec"].end(); ++it)
                    {
                        if (it.value().is_number())
                            t.events.push_back({time, TimelineEvent::Kind::Spec, it.key(), it.value().get<double>(), {}, ramp});
                        else if (it.value().is_boolean())
                            t.events.push_back({time, TimelineEvent::Kind::Spec, it.key(), it.value().get<bool>() ? 1.0 : 0.0, {}, 0.0});
                        else if (it.value().is_string())
                            t.events.push_back({time, TimelineEvent::Kind::Choice, it.key(), 0.0, it.value().get<std::string>(), 0.0});
                        else
                        {
                            error = at + "\"" + it.key() + "\": a Spec change is a number, a switch or a choice's name";
                            return false;
                        }
                    }
                }
            }
        }
        if (root.contains("assertions"))
        {
            if (!root["assertions"].is_array())
            {
                error = where + "\"assertions\" is an array";
                return false;
            }
            int index = 0;
            for (const json& a : root["assertions"])
            {
                ++index;
                const std::string at = where + "assertion " + std::to_string(index) + ": ";
                Assertion out;
                if (!a.is_object())
                {
                    error = at + "an object";
                    return false;
                }
                if (a.contains("tel"))
                {
                    out.what = "tel";
                    out.field = a.value("tel", std::string());
                }
                else if (a.contains("audio"))
                    out.what = "audio";
                else
                {
                    error = at + "it has neither \"tel\" nor \"audio\"";
                    return false;
                }
                out.measure = a.value("measure", out.what == "audio" ? a.value("audio", std::string()) : std::string("max"));
                if (a.contains("during") && a["during"].is_array() && a["during"].size() == 2)
                {
                    out.from = a["during"][0].get<double>();
                    out.to = a["during"][1].get<double>();
                }
                for (const char* op : {"<", "<=", ">", ">="})
                    if (a.contains(op) && a[op].is_number())
                    {
                        out.op = op;
                        out.value = a[op].get<double>();
                    }
                if (a.contains("within") && a["within"].is_array() && a["within"].size() == 2)
                {
                    out.op = "within";
                    out.value = a["within"][0].get<double>();
                    out.value2 = a["within"][1].get<double>();
                }
                if (out.op.empty() && !(out.what == "audio" && out.measure == "finite"))
                {
                    error = at + "it compares with nothing: \"<\", \"<=\", \">\", \">=\" or \"within\": [a, b]";
                    return false;
                }
                out.source = a.value("source", std::string());
                t.assertions.push_back(std::move(out));
            }
        }
        t.sort();
        return true;
    }

    bool loadTimeline(const std::filesystem::path& file, Timeline& timeline, std::string& error)
    {
        std::ifstream in(file, std::ios::binary);
        if (!in)
        {
            error = file.generic_string() + ": cannot be read";
            return false;
        }
        std::ostringstream text;
        text << in.rdbuf();
        return parseTimeline(text.str(), file.generic_string(), timeline, error);
    }

    bool runTimeline(SimHost& host, Build& build, const Timeline& timeline, const RunOptions& options, RunResult& result,
                     std::string& error)
    {
        const auto started = std::chrono::steady_clock::now();
        result = RunResult{};
        const double rate = host.sampleRate();
        result.rate = rate;
        const Fields& fields = host.fields();

        // the events, ramps unrolled, each value known from the one before it
        SimControls controls = host.controls();
        std::map<std::string, double> now;   // the value each ramped name had last
        std::vector<TimelineEvent> events;
        for (const TimelineEvent& e : timeline.events)
        {
            if (e.kind == TimelineEvent::Kind::Spec && build.fieldOf(e.name.substr(0, e.name.find('['))) < 0)
            {
                error = "the timeline sets '" + e.name + "', which is no Spec field of this program";
                return false;
            }
            double from = 0.0;
            const std::string key = (e.kind == TimelineEvent::Kind::Spec ? "s:" : "c:") + e.name;
            if (auto it = now.find(key); it != now.end())
                from = it->second;
            else if (e.kind == TimelineEvent::Kind::Spec)
            {
                std::size_t index = 0;
                std::string path = e.name;
                if (const std::size_t b = path.find('['); b != std::string::npos)
                {
                    index = static_cast<std::size_t>(std::strtoul(path.c_str() + b + 1, nullptr, 10));
                    path = path.substr(0, b);
                }
                from = build.get(path, index);
            }
            else
                getControl(controls, e.name, from);
            if (e.ramp > 0.0 && (e.kind == TimelineEvent::Kind::Control || e.kind == TimelineEvent::Kind::Spec))
            {
                const int steps = std::max(1, static_cast<int>(std::ceil(e.ramp / Timeline::kStep)));
                for (int k = 1; k <= steps; ++k)
                {
                    TimelineEvent step = e;
                    step.ramp = 0.0;
                    step.time = e.time + e.ramp * k / steps;
                    step.value = from + (e.value - from) * k / steps;
                    events.push_back(step);
                }
            }
            else
                events.push_back(e);
            if (e.kind == TimelineEvent::Kind::Control || e.kind == TimelineEvent::Kind::Spec)
                now[key] = e.value;
        }
        std::stable_sort(events.begin(), events.end(),
                         [](const TimelineEvent& a, const TimelineEvent& b) { return a.time < b.time; });

        // the telemetry kept: a field and an element each
        struct Kept
        {
            const FieldInfo* field;
            std::size_t index;
        };
        std::vector<Kept> kept;
        if (options.telemetry.empty())
        {
            for (const FieldInfo& f : fields.all())
            {
                if (f.block != "tel" || f.table() || f.scalar == Scalar::None || f.scalar == Scalar::Pointer)
                    continue;
                if (f.count == 1 && f.extents.empty())
                {
                    kept.push_back({&f, 0});
                    result.columns.push_back(f.path.substr(4));
                }
                else if (f.count <= 64)
                    for (std::size_t i = 0; i < f.count; ++i)
                    {
                        // the element's name: its indexes in each dimension, outermost first
                        std::string name = f.path.substr(4);
                        std::size_t rest = i, stride = f.count;
                        for (const std::size_t e : f.extents)
                        {
                            stride /= std::max<std::size_t>(e, 1);
                            name += "[" + std::to_string(rest / std::max<std::size_t>(stride, 1)) + "]";
                            rest %= std::max<std::size_t>(stride, 1);
                        }
                        kept.push_back({&f, i});
                        result.columns.push_back(name);
                    }
            }
        }
        else
            for (const std::string& p : options.telemetry)
            {
                const FieldInfo* f = nullptr;
                std::size_t index = 0;
                if (!telemetryElement(fields, p, f, index, error))
                    return false;
                kept.push_back({f, index});
                result.columns.push_back(telemetryColumn(p));
            }

        const double seconds = options.seconds > 0.0 ? options.seconds : timeline.seconds;
        const std::int64_t total = static_cast<std::int64_t>(std::llround(seconds * rate));
        const int block = std::max(1, timeline.block);
        if (options.keepAudio)
        {
            result.left.assign(static_cast<std::size_t>(total), 0.0f);
            result.right.assign(static_cast<std::size_t>(total), 0.0f);
        }
        std::vector<float> scratchL(static_cast<std::size_t>(block)), scratchR(static_cast<std::size_t>(block));
        TelemetryFrame frame;
        std::size_t next = 0;
        std::int64_t pos = 0;
        while (pos < total)
        {
            if (options.cancelled && options.cancelled())
            {
                result.cancelled = true;
                break;
            }
            bool controlsMoved = false;
            while (next < events.size() && std::llround(events[next].time * rate) <= pos)
            {
                const TimelineEvent& e = events[next++];
                bool ok = true;
                switch (e.kind)
                {
                    case TimelineEvent::Kind::Control:
                        ok = setControl(controls, e.name, e.value);
                        controlsMoved = true;
                        break;
                    case TimelineEvent::Kind::Press:
                    {
                        double v = 0.0;
                        ok = getControl(controls, e.name, v) && setControl(controls, e.name, v + 1.0);
                        controlsMoved = true;
                        break;
                    }
                    case TimelineEvent::Kind::Spec:
                    {
                        std::size_t index = 0;
                        std::string path = e.name;
                        if (const std::size_t b = path.find('['); b != std::string::npos)
                        {
                            index = static_cast<std::size_t>(std::strtoul(path.c_str() + b + 1, nullptr, 10));
                            path = path.substr(0, b);
                        }
                        ok = build.set(path, e.value, index);
                        break;
                    }
                    case TimelineEvent::Kind::Choice: ok = build.setChoice(e.name, e.text); break;
                }
                if (!ok)
                {
                    error = "the timeline's event at " + std::to_string(e.time) + " s sets '" + e.name
                            + "', which this program cannot take";
                    return false;
                }
            }
            if (controlsMoved)
                host.setControls(controls);
            build.flush();   // what the queue refused goes with the next block

            std::int64_t n = std::min<std::int64_t>(block, total - pos);
            if (next < events.size())
                n = std::min<std::int64_t>(n, std::max<std::int64_t>(1, std::llround(events[next].time * rate) - pos));
            float* l = options.keepAudio ? result.left.data() + pos : scratchL.data();
            float* r = options.keepAudio ? result.right.data() + pos : scratchR.data();
            host.process(l, r, static_cast<int>(n));
            host.service();
            pos += n;
            if (host.telemetry(frame))
            {
                const double time = static_cast<double>(pos) / rate;
                result.rowTime.push_back(time);
                for (const Kept& k : kept)
                    result.rows.push_back(static_cast<float>(frame.get(*k.field, k.index)));
                result.host = frame.host;
                if (options.afterBlock)
                    options.afterBlock(time, frame);
            }
        }
        result.simulatedSeconds = static_cast<double>(pos) / rate;
        result.stopReason = host.stopReason();
        result.wallSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        return true;
    }

    bool checkAssertions(const Timeline& timeline, const RunResult& run, std::vector<std::string>& lines)
    {
        bool all = true;
        char buffer[512];
        for (const Assertion& a : timeline.assertions)
        {
            const double to = a.to < 0.0 ? run.simulatedSeconds : a.to;
            double measured = 0.0;
            bool known = true;
            if (a.what == "tel")
            {
                const int c = run.column(a.field);
                std::vector<double> v;
                std::vector<double> t;
                if (c >= 0)
                    for (std::size_t r = 0; r < run.rowCount(); ++r)
                        if (run.rowTime[r] >= a.from && run.rowTime[r] <= to)
                        {
                            v.push_back(run.at(r, static_cast<std::size_t>(c)));
                            t.push_back(run.rowTime[r]);
                        }
                if (v.empty())
                    known = false;
                else if (a.measure == "max")
                    measured = *std::max_element(v.begin(), v.end());
                else if (a.measure == "min")
                    measured = *std::min_element(v.begin(), v.end());
                else if (a.measure == "mean")
                {
                    for (const double x : v)
                        measured += x;
                    measured /= static_cast<double>(v.size());
                }
                else if (a.measure == "last")
                    measured = v.back();
                else
                    known = false;
            }
            else
            {
                const std::size_t i0 = static_cast<std::size_t>(std::max(0.0, a.from * run.rate));
                const std::size_t i1 = std::min(run.left.size(), static_cast<std::size_t>(to * run.rate));
                std::vector<float> mid;
                for (std::size_t i = i0; i < i1; ++i)
                    mid.push_back(0.5f * (run.left[i] + run.right[i]));
                if (a.measure == "finite")
                    measured = measure::finite(run.left) && measure::finite(run.right) ? 1.0 : 0.0;
                else if (a.measure == "peak")
                    measured = std::max(measure::peak(std::span<const float>(run.left).subspan(i0, i1 - i0)),
                                        measure::peak(std::span<const float>(run.right).subspan(i0, i1 - i0)));
                else if (a.measure == "rms")
                    measured = measure::dB(measure::rms(mid));
                else if (a.measure == "step")
                    measured = measure::largestStep(mid, run.rate);
                else
                    known = false;
            }
            bool pass = known;
            if (known && a.op == "<")
                pass = measured < a.value;
            else if (known && a.op == "<=")
                pass = measured <= a.value;
            else if (known && a.op == ">")
                pass = measured > a.value;
            else if (known && a.op == ">=")
                pass = measured >= a.value;
            else if (known && a.op == "within")
                pass = measured >= a.value && measured <= a.value2;
            else if (known && a.measure == "finite")
                pass = measured == 1.0;
            all = all && pass;
            std::snprintf(buffer, sizeof buffer, "%s  %s %s %s %.3g..%.3g s %s %g%s: %s%s%s", pass ? "pass" : "FAIL",
                          a.what.c_str(), a.field.c_str(), a.measure.c_str(), a.from, to, a.op.c_str(), a.value,
                          a.op == "within" ? (" to " + std::to_string(a.value2)).c_str() : "",
                          known ? std::to_string(measured).c_str() : "not measured", a.source.empty() ? "" : "  -- ",
                          a.source.c_str());
            lines.push_back(buffer);
        }
        return all;
    }
} // namespace tee
