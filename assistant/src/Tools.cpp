// assistant/src/Tools.cpp: the tool table (Tools.h). Every touch of the document is a job on the frame's thread (Inbox.h);
// the curves run on the calling worker, on a private copy of the build.
#include "tee/assistant/Tools.h"

#include "files/BuildFiles.h"
#include "tee/Build.h"
#include "tee/Dyno.h"
#include "tee/Fields.h"
#include "tee/Predict.h"
#include "tee/Sim.h"
#include "tee/SimHost.h"
#include "tee/Telemetry.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <optional>
#include <set>
#include <thread>

namespace tee::assistant
{
    using nlohmann::json;

    namespace
    {
        constexpr double kRpmToRad = 3.14159265358979323846 / 30.0;

        Result ok(json v)
        {
            return Result{false, std::move(v)};
        }

        Result error(const std::string& why, json more = nullptr)
        {
            json v = {{"error", why}};
            if (more.is_object())
                for (auto it = more.begin(); it != more.end(); ++it)
                    v[it.key()] = it.value();
            return Result{true, std::move(v)};
        }

        Result fromReply(const Reply& r)
        {
            if (r.ok)
                return ok(r.value);
            return error(r.error, r.value);
        }

        bool stopped(const std::atomic<bool>* cancel)
        {
            return cancel != nullptr && cancel->load(std::memory_order_acquire);
        }

        // ---- the input's check: the subset of JSON Schema the table uses -------------------------------------------

        bool typeIs(const json& v, const std::string& type)
        {
            if (type == "string")
                return v.is_string();
            if (type == "number")
                return v.is_number();
            if (type == "integer")
                return v.is_number_integer() || (v.is_number() && std::isfinite(v.get<double>())
                                                 && std::trunc(v.get<double>()) == v.get<double>());
            if (type == "boolean")
                return v.is_boolean();
            if (type == "array")
                return v.is_array();
            if (type == "object")
                return v.is_object();
            return false;
        }

        // False, with `why`, when `v` is not what `schema` describes. A value of several types: "type" an array.
        bool conforms(const json& v, const json& schema, const std::string& where, std::string& why)
        {
            if (schema.contains("type"))
            {
                bool any = false;
                if (schema["type"].is_array())
                {
                    for (const json& t : schema["type"])
                        any = any || typeIs(v, t.get<std::string>());
                }
                else
                    any = typeIs(v, schema["type"].get<std::string>());
                if (!any)
                {
                    why = where + " is not of its type (" + schema["type"].dump() + ")";
                    return false;
                }
            }
            if (v.is_object() && schema.contains("properties"))
            {
                const json& props = schema["properties"];
                for (auto it = v.begin(); it != v.end(); ++it)
                    if (!props.contains(it.key()))
                    {
                        why = where + " has \"" + it.key() + "\", which this tool does not take";
                        return false;
                    }
                if (schema.contains("required"))
                    for (const json& r : schema["required"])
                        if (!v.contains(r.get<std::string>()))
                        {
                            why = where + " lacks \"" + r.get<std::string>() + "\"";
                            return false;
                        }
                for (auto it = v.begin(); it != v.end(); ++it)
                    if (!conforms(it.value(), props[it.key()], where + "." + it.key(), why))
                        return false;
            }
            if (v.is_array())
            {
                if (schema.contains("maxItems") && v.size() > schema["maxItems"].get<std::size_t>())
                {
                    why = where + " has more than " + schema["maxItems"].dump() + " items";
                    return false;
                }
                if (schema.contains("minItems") && v.size() < schema["minItems"].get<std::size_t>())
                {
                    why = where + " has fewer than " + schema["minItems"].dump() + " items";
                    return false;
                }
                if (schema.contains("items"))
                    for (std::size_t i = 0; i < v.size(); ++i)
                        if (!conforms(v[i], schema["items"], where + "[" + std::to_string(i) + "]", why))
                            return false;
            }
            if (v.is_string() && schema.contains("maxLength") && v.get<std::string>().size() > schema["maxLength"].get<std::size_t>())
            {
                why = where + " is longer than " + schema["maxLength"].dump() + " characters";
                return false;
            }
            return true;
        }

        json object(json properties, std::vector<std::string> required = {})
        {
            json s = {{"type", "object"}, {"properties", std::move(properties)}, {"additionalProperties", false}};
            if (!required.empty())
                s["required"] = required;
            return s;
        }

        // ---- the fields a tool may reach ---------------------------------------------------------------------------

        // The Spec field `path` names ("engine.bore" or "spec.engine.bore"), when a tool may reach it: a field of the
        // Spec, not hidden, not a section's rev. A field that is hidden or not in the Spec is answered as one that does
        // not exist: nothing about it is told.
        const FieldInfo* reachable(const Fields& fields, const std::string& path)
        {
            if (path.empty() || path.size() > 200 || path.find('[') != std::string::npos)
                return nullptr;
            const std::string full = path.rfind("spec.", 0) == 0 ? path : "spec." + path;
            const FieldInfo* f = fields.find(full);
            if (f == nullptr || f->block != "spec" || f->hidden || f->kind == FieldKind::Rev)
                return nullptr;
            return f;
        }

        std::string shortPath(const FieldInfo& f)
        {
            return f.path.rfind("spec.", 0) == 0 ? f.path.substr(5) : f.path;
        }

        bool scalarKind(const FieldInfo& f)
        {
            return f.kind == FieldKind::Number || f.kind == FieldKind::Integer || f.kind == FieldKind::Switch
                   || f.kind == FieldKind::Choice;
        }

        // A value as the tools give it: a number in SI, a switch as true or false, a choice by its name.
        json valueOf(const FieldInfo& f, double v)
        {
            switch (f.kind)
            {
                case FieldKind::Switch:
                    return v != 0.0;
                case FieldKind::Choice:
                {
                    const long i = std::lround(v - f.minimum.value_or(0.0));
                    if (i >= 0 && static_cast<std::size_t>(i) < f.choices.size())
                        return f.choices[static_cast<std::size_t>(i)];
                    return v;
                }
                case FieldKind::Integer:
                    return static_cast<long long>(std::llround(v));
                default:
                    return v;
            }
        }

        json currentOf(const Build& build, const FieldInfo& f)
        {
            if (!scalarKind(f))
                return nullptr;
            return valueOf(f, build.get(f.path));
        }

        json describeField(const Build& build, const FieldInfo& f)
        {
            json d = {{"path", shortPath(f)}, {"label", f.label}, {"kind", kindName(f.kind)}};
            if (!f.help.empty())
                d["help"] = f.help;
            if (!f.quantity.empty())
                d["quantity"] = f.quantity;
            if (!f.unit.empty())
                d["unit"] = f.unit;
            if (!f.group.empty())
                d["group"] = f.group;
            if (f.kind == FieldKind::Choice)
                d["choices"] = f.choices;
            else if (f.hasRange() && f.kind != FieldKind::Switch)
            {
                d["min"] = *f.minimum;
                d["max"] = *f.maximum;
            }
            if (scalarKind(f))
            {
                d["value"] = currentOf(build, f);
                d["default"] = valueOf(f, f.clamp(f.fallback()));
            }
            else
                d["note"] = "tables and arrays are not read or written by the assistant in this version";
            return d;
        }

        // The section a field is in ("engine"), from its path.
        std::string sectionOf(const FieldInfo& f)
        {
            const std::string p = shortPath(f);
            const auto dot = p.find('.');
            return dot == std::string::npos ? p : p.substr(0, dot);
        }

        // The telemetry get_status gives, by path, when the program has it: a fixed list, no more.
        const char* const kStatusTelemetry[] = {
            "tel.engine.state", "tel.engine.speed", "tel.engine.redline", "tel.engine.idle", "tel.engine.throttle",
            "tel.engine.manifold", "tel.engine.coolant", "tel.engine.torque", "tel.engine.power", "tel.car.speed",
            "tel.car.gear", "tel.boost.pressure",
        };

        std::string lower(std::string s)
        {
            for (char& c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }

        // A run of `work` on a thread of its own, stopped through `stop` when `cancel` is raised or `seconds` pass;
        // waited for either way (it stops at its next block).
        void runStoppable(const std::function<void(const std::atomic<bool>*)>& work, const std::atomic<bool>* cancel,
                          double seconds, bool& timedOut, bool& cancelled)
        {
            std::atomic<bool> stop{false};
            std::atomic<bool> done{false};
            std::thread t([&] {
                work(&stop);
                done.store(true, std::memory_order_release);
            });
            const auto deadline = std::chrono::steady_clock::now()
                                  + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seconds));
            timedOut = cancelled = false;
            while (!done.load(std::memory_order_acquire))
            {
                if (stopped(cancel))
                {
                    cancelled = true;
                    stop.store(true);
                    break;
                }
                if (std::chrono::steady_clock::now() >= deadline)
                {
                    timedOut = true;
                    stop.store(true);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            t.join();
        }
    }

    namespace
    {
        // A build of the library by the name asked, compared as the listing has it and without regard to case (the
        // rule on every platform: a disk that tells cases apart and one that does not behave alike, and a library
        // copied from one to the other never collides silently). Two listed names that differ only in case: refused,
        // ambiguous. Non-ASCII bytes compare as they are (no Unicode folding or normalisation is attempted).
        //
        // The names are the library's whole builds (BuildLibrary::wholeBuilds: GARAGE's rule, the test suite's fixtures
        // and the parts never among them), folder-qualified below the top ("archetypes/flat-plane-v8-45"). Only a
        // listed name is ever handed to the library, in the listing's own spelling: what Claude typed never becomes a
        // path.
        bool resolveBuild(const files::BuildLibrary& library, const Fields& fields, const std::string& asked,
                          std::string& found, std::string& why)
        {
            std::vector<std::string> matches;
            for (const files::WholeBuild& b : library.wholeBuilds(fields))
                if (lower(b.name) == lower(asked))
                    matches.push_back(b.name);
            if (matches.empty())
            {
                why = "the library has no build named \"" + asked + "\"";
                return false;
            }
            if (matches.size() > 1)
            {
                why = "two builds' names differ only in case (\"" + matches[0] + "\", \"" + matches[1]
                      + "\"): ask the owner to rename one in the app";
                return false;
            }
            found = matches[0];
            return true;
        }
    }

    bool safeBuildName(const std::string& name, std::string& why)
    {
        if (name.empty() || name.size() > 64)
        {
            why = "a build's name is 1 to 64 characters";
            return false;
        }
        if (name.find("..") != std::string::npos || name.front() == '.' || name.front() == '~')
        {
            why = "a build's name is a name, not a path";
            return false;
        }
        for (const char c : name)
            if (c == '/' || c == '\\' || c == ':' || static_cast<unsigned char>(c) < 32 || c == 127)
            {
                why = "a build's name is a name, not a path";
                return false;
            }
        return files::BuildLibrary::validName(name, why);
    }

    bool listedBuildName(const std::string& name, std::string& why)
    {
        if (name.empty() || name.size() > 200)
        {
            why = "a build's name is 1 to 200 characters, as list_builds gives it";
            return false;
        }
        if (name.find("..") != std::string::npos || name.front() == '/' || name.front() == '.' || name.front() == '~'
            || name.back() == '/' || name.find("//") != std::string::npos)
        {
            why = "a build's name is a name as list_builds gives it, not a path";
            return false;
        }
        for (const char c : name)
            if (c == '\\' || c == ':' || static_cast<unsigned char>(c) < 32 || c == 127)
            {
                why = "a build's name is a name as list_builds gives it, not a path";
                return false;
            }
        return true;
    }

    // ---- the table ---------------------------------------------------------------------------------------------------

    Tools::Tools(Host host) : host_(std::move(host))
    {
        const json none = object(json::object());
        const json name = {{"type", "string"}, {"maxLength", 64}, {"description", "A new build's name: plain, without a folder"}};
        const json listed = {{"type", "string"},
                             {"maxLength", 200},
                             {"description", "A build's name exactly as list_builds gives it, its folder included "
                                             "(\"v8\", \"archetypes/flat-plane-v8-45\"); letter case does not matter"}};
        infos_ = {
            {"get_status", "Status",
             "The build open in the app (its name, whether it has unsaved changes), whether the engine plays, and a "
             "snapshot of its gauges in SI units.",
             none, true},
            {"describe_field", "Describe a field",
             "One field of the build: its label, help, unit or quantity, range or choices, default and value. Paths as "
             "get_build gives them, such as \"engine.bore\".",
             object({{"path", {{"type", "string"}, {"maxLength", 200}}}}, {"path"}), true},
            {"get_build", "Read the build",
             "Without a section: the build's sections and how many fields each has. With one (such as \"engine\"): its "
             "fields' paths, labels and values, numbers in SI, switches as true or false, choices by name.",
             object({{"section", {{"type", "string"}, {"maxLength", 64}}}}), true},
            {"list_builds", "List the builds",
             "The whole builds in the app's library, as its GARAGE lists them: the shipped ones (those in a folder, "
             "such as the archetypes, named with it: \"archetypes/flat-plane-v8-45\") and the owner's own, with their "
             "layout, displacement (m^3) and boost. Parts (cams, exhausts) are not builds and are not listed.",
             none, true},
            {"set_fields", "Change fields",
             "Sets fields of the open build, all or none: one undo step in the app. Numbers in SI within the field's "
             "range (a value outside it is refused, never clamped), switches as true or false, choices by name. Tables "
             "and arrays are not set in this version.",
             object({{"changes",
                      {{"type", "array"},
                       {"minItems", 1},
                       {"maxItems", host_.limits.batchChanges},
                       {"items", object({{"path", {{"type", "string"}, {"maxLength", 200}}},
                                         {"value", {{"type", json::array({"number", "boolean", "string"})}}}},
                                        {"path", "value"})}}}},
                    {"changes"}),
             false},
            {"load_build", "Load a build",
             "Opens a whole build of the library, by its name as list_builds gives it (with its folder when it has "
             "one), in place of the one open. When the open build has unsaved changes the owner is asked in the app "
             "first.",
             object({{"name", listed}}, {"name"}), false},
            {"save_build_as", "Save as a new build",
             "Saves the open build in the owner's library under a new name. A name that is already a build's is refused: "
             "nothing is overwritten.",
             object({{"name", name}}, {"name"}), false},
            {"predict_curve", "Predict the torque curve",
             "The open build's predicted full-load torque and power against engine speed, made on a private copy (the "
             "engine playing in the app is not touched). Takes some seconds.",
             none, true},
            {"run_dyno", "Run a dyno sweep",
             "A full-throttle dyno sweep of a private copy of the open build from one engine speed to another (rpm): "
             "torque and power at each point. Takes up to 40 seconds.",
             object({{"from_rpm",
                      {{"type", "number"},
                       {"minimum", 500},
                       {"maximum", 19800},
                       {"description", "The sweep's lowest speed, rpm: 500 to 19800, and 200 to 10000 below to_rpm"}}},
                     {"to_rpm",
                      {{"type", "number"},
                       {"minimum", 700},
                       {"maximum", 20000},
                       {"description", "The sweep's highest speed, rpm: up to 20000, and 200 to 10000 above from_rpm"}}}},
                    {"from_rpm", "to_rpm"}),
             true},
        };
    }

    const ToolInfo* Tools::find(const std::string& name) const noexcept
    {
        for (const ToolInfo& t : infos_)
            if (t.name == name)
                return &t;
        return nullptr;
    }

    Result Tools::call(const std::string& name, const json& arguments, const std::atomic<bool>* cancel)
    {
        const ToolInfo* t = find(name);
        if (t == nullptr)
            return error("there is no tool named \"" + name.substr(0, 64) + "\"");
        const json args = arguments.is_null() ? json::object() : arguments;
        std::string why;
        if (!conforms(args, t->inputSchema, "arguments", why))
            return error(why);
        if (stopped(cancel))
            return error(kCancelled);
        if (name == "get_status")
            return status(args, cancel);
        if (name == "describe_field")
            return describe(args, cancel);
        if (name == "get_build")
            return getBuild(args, cancel);
        if (name == "list_builds")
            return listBuilds(args, cancel);
        if (name == "set_fields")
            return setFields(args, cancel);
        if (name == "load_build")
            return loadBuild(args, cancel);
        if (name == "save_build_as")
            return saveAs(args, cancel);
        if (name == "predict_curve")
            return predict(args, cancel);
        return dyno(args, cancel);
    }

    Result Tools::job(Inbox::Job j, const std::atomic<bool>* cancel, double seconds)
    {
        Ticket t = host_.inbox.post(std::move(j));
        return fromReply(t.wait(seconds >= 0.0 ? seconds : host_.limits.jobSeconds, cancel));
    }

    // ---- reading -----------------------------------------------------------------------------------------------------

    Result Tools::status(const json&, const std::atomic<bool>* cancel)
    {
        return job(
            [this]() -> std::optional<Reply> {
                const Build& build = host_.document.build();
                json s = {{"build", build.name}, {"unsavedChanges", host_.document.dirty()}};
                SimHost* sh = host_.simHost ? host_.simHost() : nullptr;
                TelemetryFrame frame;
                if (sh == nullptr || !sh->telemetry(frame))
                {
                    s["engine"] = "not playing yet";
                    return Reply::of(s);
                }
                s["engine"] = "playing";
                json gauges = json::object();
                const Fields& fields = sh->fields();
                for (const char* path : kStatusTelemetry)
                    if (const FieldInfo* f = fields.find(path); f != nullptr && frame.generation == fields.generation())
                    {
                        json g = {{"label", f->label}, {"value", valueOf(*f, frame.get(*f))}};
                        if (!f->quantity.empty())
                            g["quantity"] = f->quantity;
                        if (!f->unit.empty())
                            g["unit"] = f->unit;
                        gauges[std::string(path).substr(4)] = g;
                    }
                s["gauges"] = gauges;
                s["units"] = "SI: rotation in rad/s, pressure in Pa absolute, temperature in K, speed in m/s";
                return Reply::of(s);
            },
            cancel);
    }

    Result Tools::describe(const json& a, const std::atomic<bool>* cancel)
    {
        const std::string path = a["path"].get<std::string>();
        return job(
            [this, path]() -> std::optional<Reply> {
                const Build& build = host_.document.build();
                const FieldInfo* f = reachable(build.fields(), path);
                if (f == nullptr)
                    return Reply::failed("the build has no field \"" + path.substr(0, 80) + "\"");
                return Reply::of(describeField(build, *f));
            },
            cancel);
    }

    Result Tools::getBuild(const json& a, const std::atomic<bool>* cancel)
    {
        const std::string section = a.contains("section") ? a["section"].get<std::string>() : std::string();
        return job(
            [this, section]() -> std::optional<Reply> {
                const Build& build = host_.document.build();
                const Fields& fields = build.fields();
                json out = {{"build", build.name}, {"unsavedChanges", host_.document.dirty()}};
                if (section.empty())
                {
                    json sections = json::object();
                    for (const FieldInfo& f : fields.all())
                        if (reachable(fields, f.path) == &f)
                        {
                            json& s = sections[sectionOf(f)];
                            s = s.is_null() ? 1 : s.get<int>() + 1;
                        }
                    out["sections"] = sections;
                    out["note"] = "ask again with a section's name for its fields";
                    return Reply::of(out);
                }
                json list = json::array();
                const std::string want = section.rfind("spec.", 0) == 0 ? section.substr(5) : section;
                for (const FieldInfo& f : fields.all())
                    if (reachable(fields, f.path) == &f && sectionOf(f) == want)
                    {
                        json e = {{"path", shortPath(f)}, {"label", f.label}};
                        if (scalarKind(f))
                            e["value"] = currentOf(build, f);
                        else
                            e["kind"] = kindName(f.kind);
                        if (!f.quantity.empty())
                            e["quantity"] = f.quantity;
                        else if (!f.unit.empty())
                            e["unit"] = f.unit;
                        list.push_back(std::move(e));
                    }
                if (list.empty())
                    return Reply::failed("the build has no section \"" + want.substr(0, 64) + "\"");
                out["section"] = want;
                out["fields"] = std::move(list);
                return Reply::of(out);
            },
            cancel);
    }

    Result Tools::listBuilds(const json&, const std::atomic<bool>* cancel)
    {
        return job(
            [this]() -> std::optional<Reply> {
                json list = json::array();
                for (const files::WholeBuild& w : host_.library.wholeBuilds(host_.document.build().fields()))
                {
                    files::BuildSummary s;
                    if (!files::readSummary(w.file, s))
                        continue;
                    json e = {{"name", w.name}, {"own", w.user}};
                    if (!s.title.empty() && s.title != s.name && s.title != w.name)
                        e["title"] = s.title;
                    if (!s.layout.empty())
                        e["layout"] = s.layout;
                    if (s.displacement > 0.0)
                        e["displacement"] = s.displacement;
                    if (!s.boost.empty())
                        e["boost"] = s.boost;
                    list.push_back(std::move(e));
                }
                return Reply::of({{"builds", std::move(list)}});
            },
            cancel);
    }

    // ---- acting ------------------------------------------------------------------------------------------------------

    Result Tools::setFields(const json& a, const std::atomic<bool>* cancel)
    {
        struct Wanted
        {
            std::string path;
            json value;
        };
        std::vector<Wanted> wanted;
        for (const json& c : a["changes"])
            wanted.push_back({c["path"].get<std::string>(), c["value"]});
        auto since = std::make_shared<double>(-1.0);
        const double wait = host_.limits.gestureSeconds + host_.limits.jobSeconds;
        return job(
            [this, wanted, since]() -> std::optional<Reply> {
                Build& build = host_.document.build();
                const Fields& fields = build.fields();
                // every change checked before any is made
                struct Change
                {
                    const FieldInfo* f;
                    double value;
                };
                std::vector<Change> changes;
                std::set<std::string> seen;
                for (const Wanted& w : wanted)
                {
                    const FieldInfo* f = reachable(fields, w.path);
                    if (f == nullptr)
                        return Reply::failed("the build has no field \"" + w.path.substr(0, 80) + "\"; nothing was changed");
                    if (!seen.insert(f->path).second)
                        return Reply::failed("\"" + shortPath(*f) + "\" is named twice; nothing was changed");
                    const json stored = {{"path", shortPath(*f)}, {"stored", currentOf(build, *f)}};
                    if (!scalarKind(*f))
                        return Reply::failed("\"" + shortPath(*f) + "\" is a table or an array, which this version does not set; "
                                             "nothing was changed", stored);
                    double v = 0.0;
                    if (f->kind == FieldKind::Choice)
                    {
                        if (!w.value.is_string() || f->choiceIndex(w.value.get<std::string>()) < 0)
                        {
                            json more = stored;
                            more["choices"] = f->choices;
                            return Reply::failed("\"" + shortPath(*f) + "\" is a choice: give one of its choices by name; "
                                                 "nothing was changed", more);
                        }
                        v = f->minimum.value_or(0.0) + f->choiceIndex(w.value.get<std::string>());
                    }
                    else if (f->kind == FieldKind::Switch)
                    {
                        if (w.value.is_boolean())
                            v = w.value.get<bool>() ? 1.0 : 0.0;
                        else if (w.value.is_number() && (w.value.get<double>() == 0.0 || w.value.get<double>() == 1.0))
                            v = w.value.get<double>();
                        else
                            return Reply::failed("\"" + shortPath(*f) + "\" is a switch: true or false; nothing was changed",
                                                 stored);
                    }
                    else
                    {
                        if (!w.value.is_number() || !std::isfinite(w.value.get<double>()))
                            return Reply::failed("\"" + shortPath(*f) + "\" takes a number; nothing was changed", stored);
                        v = w.value.get<double>();
                        if (f->kind == FieldKind::Integer && std::trunc(v) != v)
                            return Reply::failed("\"" + shortPath(*f) + "\" takes a whole number; nothing was changed", stored);
                        if (f->hasRange())
                        {
                            const double lo = std::min(*f->minimum, *f->maximum), hi = std::max(*f->minimum, *f->maximum);
                            if (v < lo || v > hi)
                            {
                                json more = stored;
                                more["min"] = lo;
                                more["max"] = hi;
                                more["asked"] = v;
                                return Reply::failed("\"" + shortPath(*f) + "\" is outside its range; nothing was changed",
                                                     more);
                            }
                        }
                    }
                    changes.push_back({f, v});
                }
                // a control held in the app: the batch waits (it would join the person's gesture), for a while
                const double now = host_.clock ? host_.clock() : 0.0;
                if (*since < 0.0)
                    *since = now;
                if (build.gestureOpen())
                {
                    if (now - *since < host_.limits.gestureSeconds)
                        return std::nullopt;
                    return Reply::failed("a control is being moved in the app; nothing was changed: try again");
                }
                json done = json::array();
                build.beginGesture();
                for (const Change& c : changes)
                {
                    const json before = currentOf(build, *c.f);
                    build.set(c.f->path, c.value);
                    done.push_back({{"path", shortPath(*c.f)}, {"before", before}, {"after", currentOf(build, *c.f)}});
                }
                const std::uint64_t had = build.undoStep();
                build.endGesture();
                const std::uint64_t step = build.undoStep();
                json out = {{"changed", std::move(done)}};
                if (step != had && step != 0)
                {
                    out["undoStep"] = step;
                    out["note"] = "one undo step in the app";
                }
                else
                    out["note"] = "every value was already so: nothing changed";
                return Reply::of(out);
            },
            cancel, wait);
    }

    Result Tools::loadBuild(const json& a, const std::atomic<bool>* cancel)
    {
        const std::string name = a["name"].get<std::string>();
        std::string why;
        if (!listedBuildName(name, why))
            return error(why);
        struct Asked
        {
            std::future<bool> answer;
            std::uint64_t hash = 0;
            std::string open;
            std::shared_ptr<std::atomic<bool>> withdrawn = std::make_shared<std::atomic<bool>>(false);
        };
        auto asked = std::make_shared<Asked>();
        // however this call ends once it has asked (answered, out of time, cancelled, stopped), the question is
        // withdrawn: the app takes it down if it is still showing, and never shows it if it is still queued
        struct Withdraw
        {
            std::shared_ptr<std::atomic<bool>> flag;
            ~Withdraw() { flag->store(true, std::memory_order_release); }
        } const withdraw{asked->withdrawn};
        auto chosen = std::make_shared<std::string>(name);   // the library's own spelling of it
        const auto load = [this, chosen]() -> Reply {
            const std::string& name = *chosen;
            std::vector<std::string> warnings;
            std::string err;
            if (!host_.library.load(host_.document, name, warnings, err))
                return Reply::failed("the build \"" + name + "\" could not be read; nothing was loaded");   // (err may name a file: never shown)
            json out = {{"loaded", name}, {"undoStep", host_.document.build().undoStep()}};
            if (!warnings.empty())
                out["warnings"] = warnings.size();
            return Reply::of(out);
        };
        Result first = job(
            [this, name, asked, load, chosen]() -> std::optional<Reply> {
                std::string found, whyNot;
                if (!resolveBuild(host_.library, host_.document.build().fields(), name, found, whyNot))
                    return Reply::failed(whyNot);
                *chosen = found;
                files::BuildSummary s;
                if (!host_.library.summary(found, s))
                    return Reply::failed("the library has no build named \"" + name + "\"");
                if (!s.part.empty())
                    return Reply::failed("\"" + found + "\" is a part (" + s.part + "), not a whole build");
                if (!host_.document.dirty())
                    return load();
                if (!host_.ask)
                    return Reply::failed("the open build has unsaved changes and the owner cannot be asked; nothing was loaded");
                asked->hash = host_.document.build().hash();
                asked->open = host_.document.build().name;
                asked->answer = host_.ask("Claude asks to load \"" + found + "\". The open build, \"" + asked->open
                                              + "\", has unsaved changes, which would be lost. Load it?",
                                          asked->withdrawn);
                return Reply::of({{"asking", true}});
            },
            cancel);
        if (first.isError || !first.value.contains("asking"))
            return first;
        // the owner's answer, waited for here (never in a job)
        const auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                  std::chrono::duration<double>(host_.limits.askSeconds));
        bool answered = false;
        while (!answered)
        {
            if (stopped(cancel))
                return error("cancelled while the owner was asked; nothing was loaded");
            if (std::chrono::steady_clock::now() >= deadline)
                return error("the owner has not answered, and the question has been taken off their screen: nothing was "
                             "loaded. Do not ask again until the owner says they are ready.");
            answered = asked->answer.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready;
        }
        if (!asked->answer.get())
            return error("the owner said no; nothing was loaded");
        return job(
            [this, asked, load]() -> std::optional<Reply> {
                if (host_.document.build().hash() != asked->hash || host_.document.build().name != asked->open)
                    return Reply::failed("the build changed while the owner was asked; nothing was loaded");
                return load();
            },
            cancel);
    }

    Result Tools::saveAs(const json& a, const std::atomic<bool>* cancel)
    {
        const std::string name = a["name"].get<std::string>();
        std::string why;
        if (!safeBuildName(name, why))
            return error(why);
        for (const char c : name)
            if (static_cast<unsigned char>(c) >= 0x80)
                return error("a new build's name is written in plain ASCII here, so that names compare the same on every "
                             "system (letters without accents, digits, spaces, - _ ( ) .)");
        return job(
            [this, name]() -> std::optional<Reply> {
                // a name already a build's, shipped or own, in any letter case (a Mac's disk does not tell them apart)
                files::BuildSummary s;
                bool taken = host_.library.summary(name, s);
                for (const files::BuildSummary& b : host_.library.list())
                    taken = taken || lower(b.name) == lower(name);
                if (taken)
                    return Reply::failed("a build named \"" + name + "\" exists; choose a new name (nothing is overwritten)");
                std::string err;
                if (!host_.library.save(host_.document, name, err))
                    return Reply::failed("the build could not be saved as \"" + name + "\"");
                return Reply::of({{"saved", name}});
            },
            cancel);
    }

    // ---- the curves, on a private copy -------------------------------------------------------------------------------

    namespace
    {
        // The open build as text, taken in a job, and the program to run it on.
        struct Copy
        {
            std::shared_ptr<Sim> sim;
            std::string text;
        };
    }

    Result Tools::predict(const json&, const std::atomic<bool>* cancel)
    {
        auto copy = std::make_shared<Copy>();
        Result taken = job(
            [this, copy]() -> std::optional<Reply> {
                copy->sim = host_.sim ? host_.sim() : nullptr;
                if (!copy->sim)
                    return Reply::failed("the engine is not ready yet");
                copy->text = host_.document.build().save();
                return Reply::of(nullptr);
            },
            cancel);
        if (taken.isError)
            return taken;
        Build build(copy->sim->fields());
        std::vector<std::string> warnings;
        std::string err;
        if (!build.load(copy->text, BuildPart::Whole, warnings, err))
            return error("the build could not be copied for the prediction");
        PredictCurve curve;
        bool okRun = false, timedOut = false, cancelled = false, yielded = true;
        const auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                  std::chrono::duration<double>(host_.limits.predictSeconds));
        runStoppable(
            [&](const std::atomic<bool>* stop) {
                if (!(yielded = yieldToOthers(stop, cancel, deadline)))
                    return;
                running_ = true;
                PredictOptions o;
                o.onPoint = [&, stop] {
                    ++points_;
                    yieldToOthers(stop, cancel, deadline);
                };
                okRun = predictCurve(copy->sim, build, o, curve, err, stop);
                running_ = false;
            },
            cancel, host_.limits.predictSeconds, timedOut, cancelled);
        if (cancelled)
            return error("cancelled");
        if (!yielded)
            return error("the app is busy with its own curve or calibration; try again in a while");
        if (timedOut)
            return error("the prediction took too long and was stopped");
        if (!okRun)
            return error("this build cannot be put on the predictor");
        json points = json::array();
        double peakTorque = 0.0, peakTorqueRpm = 0.0, peakPower = 0.0, peakPowerRpm = 0.0;
        for (const PredictPoint& p : curve.points)
        {
            if (!std::isfinite(p.torque) || !std::isfinite(p.power))
                continue;
            const double rpm = p.speed / kRpmToRad;
            points.push_back({{"rpm", std::round(rpm)}, {"torqueNm", std::round(p.torque * 10.0) / 10.0},
                              {"powerKW", std::round(p.power / 100.0) / 10.0}, {"settled", p.settled}});
            if (p.torque > peakTorque)
                peakTorque = p.torque, peakTorqueRpm = rpm;
            if (p.power > peakPower)
                peakPower = p.power, peakPowerRpm = rpm;
        }
        return ok({{"points", std::move(points)},
                   {"peakTorque", {{"Nm", std::round(peakTorque * 10.0) / 10.0}, {"rpm", std::round(peakTorqueRpm)}}},
                   {"peakPower", {{"kW", std::round(peakPower / 100.0) / 10.0}, {"rpm", std::round(peakPowerRpm)}}},
                   {"note", "full throttle, a warm engine held at each speed; the build's copy, not the engine playing"}});
    }

    Result Tools::dyno(const json& a, const std::atomic<bool>* cancel)
    {
        const double from = a["from_rpm"].get<double>(), to = a["to_rpm"].get<double>();
        if (!std::isfinite(from) || !std::isfinite(to) || from < 500.0 || to > 20000.0 || to - from < 200.0
            || to - from > 10000.0)
            return error("from_rpm and to_rpm: 500 to 20000, from below to by 200 to 10000");
        auto copy = std::make_shared<Copy>();
        Result taken = job(
            [this, copy]() -> std::optional<Reply> {
                copy->sim = host_.sim ? host_.sim() : nullptr;
                if (!copy->sim)
                    return Reply::failed("the engine is not ready yet");
                copy->text = host_.document.build().save();
                return Reply::of(nullptr);
            },
            cancel);
        if (taken.isError)
            return taken;
        Build build(copy->sim->fields());
        std::vector<std::string> warnings;
        std::string err;
        if (!build.load(copy->text, BuildPart::Whole, warnings, err))
            return error("the build could not be copied for the dyno");
        DynoPlan plan;
        plan.from = from * kRpmToRad;
        plan.to = to * kRpmToRad;
        plan.spacing = std::max(10.0, (plan.to - plan.from) / 16.0);
        DynoCurve curve;
        bool okRun = false, timedOut = false, cancelled = false;
        bool yielded = true;
        const auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                  std::chrono::duration<double>(host_.limits.dynoSeconds));
        runStoppable(
            [&](const std::atomic<bool>* stop) {
                if (!(yielded = yieldToOthers(stop, cancel, deadline)))
                    return;
                running_ = true;
                // the dyno reports its progress now and then: the place it waits while the app's own work runs
                okRun = runDyno(copy->sim, build, plan, curve, err,
                                [&, stop](double) {
                                    ++points_;
                                    yieldToOthers(stop, cancel, deadline);
                                },
                                stop);
                running_ = false;
            },
            cancel, host_.limits.dynoSeconds, timedOut, cancelled);
        if (cancelled)
            return error("cancelled");
        if (!yielded)
            return error("the app is busy with its own curve or calibration; try again in a while");
        if (timedOut)
            return error("the dyno run took too long and was stopped");
        if (!okRun)
            return error("this build cannot be put on the dyno");
        json points = json::array();
        for (const DynoPoint& p : curve.points)
            if (std::isfinite(p.torque) && std::isfinite(p.power))
                points.push_back({{"rpm", std::round(p.speed / kRpmToRad)}, {"torqueNm", std::round(p.torque * 10.0) / 10.0},
                                  {"powerKW", std::round(p.power / 100.0) / 10.0}});
        json out = {{"points", std::move(points)}, {"note", "a full-throttle sweep of the build's copy"}};
        if (curve.faults > 0)
            out["faults"] = curve.faults;
        return ok(out);
    }

    bool Tools::yieldToOthers(const std::atomic<bool>* stop, const std::atomic<bool>* cancel,
                              std::chrono::steady_clock::time_point deadline)
    {
        if (!host_.othersBusy)
            return true;
        while (host_.othersBusy())
        {
            if (stopped(stop) || stopped(cancel) || std::chrono::steady_clock::now() >= deadline)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return true;
    }

    // ---- who drives, and a turn's limits -----------------------------------------------------------------------------

    const char* driverName(Driver d) noexcept
    {
        return d == Driver::Chat ? "the chat" : "Claude Desktop";
    }

    bool Drivers::take(Driver who, std::string& why)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ > 0 && holder_ != who)
        {
            why = std::string(driverName(holder_)) + " is using the build now; try again when it is done";
            return false;
        }
        holder_ = who;
        ++count_;
        return true;
    }

    void Drivers::release(Driver who)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ > 0 && holder_ == who)
            --count_;
    }

    bool Drivers::held() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ > 0;
    }

    Turn::Turn(Tools& tools, Drivers& drivers, Driver who, std::function<double()> clock)
        : tools_(tools), drivers_(drivers), who_(who), clock_(std::move(clock))
    {
        active_ = drivers_.take(who_, why_);
        started_ = clock_ ? clock_() : 0.0;
    }

    Turn::~Turn()
    {
        if (active_)
            drivers_.release(who_);
    }

    Result Turn::call(const std::string& name, const json& arguments, const std::atomic<bool>* cancel)
    {
        const Limits& l = tools_.limits();
        if (!active_)
            return error(why_);
        if (calls_ >= l.turnCalls)
            return error("this turn has made its " + std::to_string(l.turnCalls) + " tool calls; answer with what you have");
        if (clock_ && clock_() - started_ > l.turnSeconds)
            return error("this turn has run out of time; answer with what you have");
        if (bytes_ >= l.turnBytes)
            return error("this turn's tool results are at their limit; answer with what you have");
        ++calls_;
        Result r = tools_.call(name, arguments, cancel);
        std::size_t n = r.text().size();
        if (n > l.resultBytes)
        {
            r = error("the result was too large (" + std::to_string(n) + " bytes); ask for less, such as one section");
            n = r.text().size();
        }
        bytes_ += n;
        return r;
    }
}
