// host/src/Validation.cpp: validation files and their run (Validation.h).
#include "tee/Validation.h"

#include "tee/Predict.h"

#include "tee/Measures.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace tee
{
    bool Validation::parse(std::string_view text, std::string_view fileName, Validation& v, std::string& error)
    {
        const std::string where = std::string(fileName) + ": ";
        const nlohmann::json root = nlohmann::json::parse(text, nullptr, false);
        if (root.is_discarded() || root.value("format", std::string()) != "tee-validation")
        {
            error = where + "not a validation (\"format\": \"tee-validation\")";
            return false;
        }
        v = Validation{};
        v.build = root.value("build", std::string());
        v.description = root.value("description", std::string());
        int index = 0;
        for (const nlohmann::json& j : root.value("targets", nlohmann::json::array()))
        {
            ++index;
            const std::string at = where + "target " + std::to_string(index) + ": ";
            ValidationTarget t;
            t.name = j.value("name", "target " + std::to_string(index));
            t.scenario = j.value("scenario", std::string());
            t.tel = j.value("tel", std::string());
            t.audio = j.value("audio", std::string());
            t.measure = j.value("measure", t.audio.empty() ? std::string("mean") : t.audio);
            t.unit = j.value("unit", std::string());
            t.source = j.value("source", std::string());
            t.fast = j.value("fast", false);
            t.at = j.value("at", 0.0);
            t.relativeTo = j.value("relativeTo", std::string());
            if (j.contains("during") && j["during"].is_array() && j["during"].size() == 2)
            {
                t.from = j["during"][0].get<double>();
                t.to = j["during"][1].get<double>();
            }
            if (!j.contains("range") || !j["range"].is_array() || j["range"].size() != 2)
            {
                error = at + "a \"range\": [low, high]";
                return false;
            }
            t.low = j["range"][0].get<double>();
            t.high = j["range"][1].get<double>();
            if (j.contains("dyno"))
            {
                const nlohmann::json& d = j["dyno"];
                t.dyno = true;
                t.plan.from = d.value("from", t.plan.from);
                t.plan.to = d.value("to", t.plan.to);
                t.plan.rate = d.value("rate", t.plan.rate);
                t.plan.spacing = d.value("spacing", t.plan.spacing);
                t.plan.settle = d.value("settle", t.plan.settle);
                t.plan.measure = d.value("measure", t.plan.measure);
                t.plan.pedal = d.value("pedal", t.plan.pedal);
                t.plan.warmup = d.value("warmup", t.plan.warmup);
                t.sweep = d.value("sweep", false);
                t.plan.torque = d.value("torque", t.plan.torque);
                t.plan.chassis = d.value("chassis", false);
                for (const nlohmann::json& s : d.value("steps", nlohmann::json::array()))
                    t.plan.steps.push_back(s.get<double>());
            }
            else if (t.scenario.empty())
            {
                error = at + "it names neither a \"scenario\" nor a \"dyno\"";
                return false;
            }
            if (t.source.empty())
            {
                error = at + "it says no \"source\": where its range comes from, in words";
                return false;
            }
            v.targets.push_back(std::move(t));
        }
        return true;
    }

    bool Validation::loadFile(const std::filesystem::path& file, Validation& v, std::string& error)
    {
        std::ifstream in(file, std::ios::binary);
        if (!in)
        {
            error = file.generic_string() + ": cannot be read";
            return false;
        }
        std::ostringstream text;
        text << in.rdbuf();
        return parse(text.str(), file.generic_string(), v, error);
    }

    namespace
    {
        std::filesystem::path named(const std::filesystem::path& content, const std::string& sub, const std::string& name)
        {
            std::filesystem::path p = name;
            if (p.extension() == ".json" || name.find('/') != std::string::npos)
                return std::filesystem::exists(p) ? p : content / p;
            return content / sub / (name + ".json");
        }
    } // namespace

    int referenceSettings(Build& build)
    {
        int changed = 0;
        const Fields& fields = build.fields();
        for (std::size_t i = 0; i < fields.all().size(); ++i)
        {
            const FieldInfo& f = fields.all()[i];
            if (f.block != "spec" || f.count != 1 || (f.scalar != Scalar::Bool && f.scalar != Scalar::Int))
                continue;
            const std::string name = f.path.substr(f.path.rfind('.') + 1);
            const bool isSwitch = name == "model" || name == "custom" || f.help.rfind("Off: the reference", 0) == 0;
            if (isSwitch && build.get(f.path) != 0.0)
            {
                build.set(f.path, 0.0);
                ++changed;
            }
        }
        return changed;
    }

    bool runValidation(const std::shared_ptr<Sim>& sim, const Validation& v, const std::filesystem::path& content,
                       bool fastOnly, std::vector<ValidationRow>& rows, std::string& error,
                       const std::function<void(Build&)>& adjust)
    {
        rows.clear();
        Build build(sim->fields());
        if (!v.build.empty())
        {
            std::vector<std::string> warnings;
            if (!build.loadFile(named(content, "builds", v.build), BuildPart::Whole, warnings, error))
                return false;
        }
        if (adjust)
            adjust(build);
        std::map<std::string, RunResult> runs;
        std::map<std::uint64_t, DynoCurve> curves;
        std::set<std::uint64_t> hostBraked;
        for (const ValidationTarget& t : v.targets)
        {
            if (fastOnly && !t.fast)
                continue;
            ValidationRow row;
            row.target = &t;
            if (t.dyno)
            {
                const std::uint64_t key = t.plan.hash();
                if (curves.find(key) == curves.end())
                {
                    DynoCurve curve;
                    std::string why;
                    if (t.plan.steps.empty() && !t.sweep)
                    {
                        // the predicted curve's steady points (Predict.h): a warm engine, its exhaust settled, every
                        // 15 rad/s or so over the plan's speeds (a sweep's curve depends on how long it took)
                        PredictOptions o;
                        o.from = t.plan.from;
                        o.to = t.plan.to;
                        o.pedal = t.plan.pedal;
                        o.points = std::clamp(static_cast<int>((t.plan.to - t.plan.from) / 15.0) + 1, 8, 48);
                        PredictCurve pc;
                        if (!predictCurve(sim, build, o, pc, why))
                        {
                            row.why = "not measurable: " + why;
                            rows.push_back(row);
                            continue;
                        }
                        for (const PredictPoint& p : pc.points)
                            curve.points.push_back({p.speed, p.torque, p.power, {}});
                        curve.simulatedSeconds = pc.simulatedSeconds;
                        curve.wallSeconds = pc.wallSeconds;
                        hostBraked.insert(key);
                    }
                    else if (!runDyno(sim, build, t.plan, curve, why))
                    {
                        row.why = "not measurable: " + why;
                        rows.push_back(row);
                        continue;
                    }
                    curves[key] = curve;
                }
                const DynoCurve& c = curves[key];
                if (hostBraked.count(key) != 0)
                    row.how = "steady points";
                if (c.points.empty())
                    row.why = "not measurable: the dyno gave no points";
                else
                {
                    const auto best = [&](bool power) {
                        return *std::max_element(c.points.begin(), c.points.end(), [&](const DynoPoint& a, const DynoPoint& b) {
                            return power ? a.power < b.power : a.torque < b.torque;
                        });
                    };
                    if (t.measure == "maxTorque")
                        row.measured = best(false).torque;
                    else if (t.measure == "maxPower")
                        row.measured = best(true).power;
                    else if (t.measure == "speedAtMaxTorque")
                        row.measured = best(false).speed;
                    else if (t.measure == "speedAtMaxPower")
                        row.measured = best(true).speed;
                    else if (t.measure == "torqueAt")
                    {
                        const DynoPoint* near = &c.points.front();
                        for (const DynoPoint& p : c.points)
                            if (std::fabs(p.speed - t.at) < std::fabs(near->speed - t.at))
                                near = &p;
                        row.measured = near->torque;
                    }
                    else
                        row.why = "no dyno measure \"" + t.measure + "\"";
                }
            }
            else
            {
                const FieldInfo* field = nullptr;
                std::size_t element = 0;
                std::string why;
                if (!t.tel.empty() && !telemetryElement(sim->fields(), t.tel, field, element, why))
                {
                    row.why = "not measurable: " + why;
                    rows.push_back(row);
                    continue;
                }
                auto it = runs.find(t.scenario);
                if (it == runs.end())
                {
                    Timeline timeline;
                    if (!loadTimeline(named(content, "scenarios", t.scenario), timeline, error))
                        return false;
                    Build copy(sim->fields());
                    std::vector<std::string> warnings;
                    const std::string buildName = timeline.build.empty() ? std::string() : timeline.build;
                    if (!copy.load(build.save(), BuildPart::Whole, warnings, error))
                        return false;
                    if (!buildName.empty() && !copy.loadFile(named(content, "builds", buildName), BuildPart::Whole,
                                                             warnings, error))
                        return false;
                    SimHost host;
                    LoadOptions load;
                    load.sim = sim->options();
                    load.sampleRate = timeline.rate;
                    load.maxBlock = std::max(16, timeline.block);
                    load.worker = false;
                    if (timeline.warm)
                        load.hold = warmEngine();
                    if (!host.attach(sim, load, error))
                        return false;
                    copy.attach(&host);
                    RunResult run;
                    if (!runTimeline(host, copy, timeline, {}, run, error))
                        return false;
                    it = runs.emplace(t.scenario, std::move(run)).first;
                }
                const RunResult& run = it->second;
                const double to = t.to < 0.0 ? run.simulatedSeconds : t.to;
                if (!t.relativeTo.empty())
                {
                    const int c = run.column(t.relativeTo);
                    if (c < 0 || run.rowCount() == 0 || run.at(run.rowCount() - 1, static_cast<std::size_t>(c)) == 0.0)
                    {
                        row.why = "not measurable: no tel." + t.relativeTo + " to measure it against";
                        rows.push_back(row);
                        continue;
                    }
                }
                if (!t.tel.empty())
                {
                    const int c = run.column(t.tel);
                    std::vector<double> values;
                    for (std::size_t r = 0; c >= 0 && r < run.rowCount(); ++r)
                        if (run.rowTime[r] >= t.from && run.rowTime[r] <= to)
                            values.push_back(run.at(r, static_cast<std::size_t>(c)));
                    if (values.empty())
                        row.why = "not measurable: no rows in its window";
                    else if (t.measure == "max")
                        row.measured = *std::max_element(values.begin(), values.end());
                    else if (t.measure == "min")
                        row.measured = *std::min_element(values.begin(), values.end());
                    else if (t.measure == "last")
                        row.measured = values.back();
                    else
                    {
                        double s = 0.0;
                        for (const double x : values)
                            s += x;
                        row.measured = s / static_cast<double>(values.size());
                    }
                }
                else
                {
                    const std::size_t i0 = std::min(run.left.size(), static_cast<std::size_t>(t.from * run.rate));
                    const std::size_t i1 = std::min(run.left.size(), static_cast<std::size_t>(to * run.rate));
                    std::vector<float> mid;
                    for (std::size_t i = i0; i < i1; ++i)
                        mid.push_back(0.5f * (run.left[i] + run.right[i]));
                    if (t.audio == "peak")
                        row.measured = measure::peak(mid);
                    else if (t.audio == "rms")
                        row.measured = measure::dB(measure::rms(mid));
                    else if (t.audio == "step")
                        row.measured = measure::largestStep(mid, run.rate);
                    else
                        row.why = "no audio measure \"" + t.audio + "\"";
                }
            }
            if (row.measured && !t.dyno && !t.relativeTo.empty())
            {
                const RunResult& run = runs[t.scenario];
                const int c = run.column(t.relativeTo);
                row.measured = *row.measured / run.at(run.rowCount() - 1, static_cast<std::size_t>(c));
            }
            if (row.measured)
                row.pass = *row.measured >= t.low && *row.measured <= t.high;
            rows.push_back(row);
        }
        return true;
    }

    std::string validationTable(const std::vector<ValidationRow>& rows)
    {
        std::string out;
        char line[512];
        std::snprintf(line, sizeof line, "%-28s %-30s %-12s %-6s %s\n", "target", "range", "measured", "pass", "source");
        out += line;
        for (const ValidationRow& r : rows)
        {
            const ValidationTarget& t = *r.target;
            char range[64], measured[64];
            std::snprintf(range, sizeof range, "%.4g to %.4g %s", t.low, t.high, t.unit.c_str());
            if (r.measured)
                std::snprintf(measured, sizeof measured, "%.4g", *r.measured);
            else
                std::snprintf(measured, sizeof measured, "-");
            std::snprintf(line, sizeof line, "%-28s %-30s %-12s %-6s %s%s%s\n", t.name.c_str(), range, measured,
                          r.measured ? (r.pass ? "pass" : "FAIL") : "n/a", t.source.c_str(),
                          !r.why.empty() ? "  -- " : !r.how.empty() ? "  -- " : "", !r.why.empty() ? r.why.c_str() : r.how.c_str());
            out += line;
        }
        return out;
    }

    std::string validationComparison(const std::vector<ValidationRow>& reference, const std::vector<ValidationRow>& simulator,
                                     bool same)
    {
        const auto cell = [](const ValidationRow& r) {
            char text[64];
            if (!r.measured)
                return std::string("n/a");
            std::snprintf(text, sizeof text, "%.4g %s", *r.measured, r.pass ? "pass" : "MISS");
            return std::string(text);
        };
        std::string out;
        char line[640];
        std::snprintf(line, sizeof line, "%-24s %-26s %-16s %-16s %s\n", "target", "range", "reference",
                      same ? "simulator (=)" : "simulator", "source");
        out += line;
        std::vector<std::string> refMiss, simMiss;
        for (std::size_t i = 0; i < simulator.size(); ++i)
        {
            const ValidationRow& s = simulator[i];
            const ValidationRow* r = nullptr;
            for (const ValidationRow& candidate : reference)
                if (candidate.target == s.target)
                    r = &candidate;
            const ValidationTarget& t = *s.target;
            char range[64];
            std::snprintf(range, sizeof range, "%.4g to %.4g %s", t.low, t.high, t.unit.c_str());
            const std::string note = !s.why.empty() ? "  -- " + s.why : !s.how.empty() ? "  (" + s.how + ")" : "";
            std::snprintf(line, sizeof line, "%-24s %-26s %-16s %-16s %s%s\n", t.name.c_str(), range,
                          r != nullptr ? cell(*r).c_str() : "-", cell(s).c_str(), t.source.c_str(), note.c_str());
            out += line;
            if (r != nullptr && r->measured && !r->pass)
                refMiss.push_back(t.name);
            if (s.measured && !s.pass)
                simMiss.push_back(t.name);
        }
        const auto list = [](const std::vector<std::string>& names) {
            std::string text;
            for (const std::string& n : names)
                text += (text.empty() ? "" : ", ") + n;
            return text.empty() ? std::string("none") : text;
        };
        out += "the reference misses: " + list(refMiss) + "\n";
        out += "the simulator at its defaults misses: " + list(simMiss)
               + (same ? " (the same build: no model switch is on)" : "") + "\n";
        return out;
    }
} // namespace tee
