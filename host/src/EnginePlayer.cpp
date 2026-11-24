// host/src/EnginePlayer.cpp: tee::EnginePlayer, builds, engine scenarios and their render (EnginePlayer.h).
#include "tee/EnginePlayer.h"

#include "tee/Denormals.h"

#include <cstdlib>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <optional>
#include <sstream>

namespace tee
{
    namespace
    {
        using Json = nlohmann::ordered_json;

        // The loop budget, as RefPlayer sets it: 2^32 iterations a second of samples, 16 samples' worth at least.
        constexpr double kLoopsPerSecond = 4294967296.0;
        constexpr int kBudgetFloor = 16;
        constexpr std::int64_t kInitBudget = std::int64_t{1} << 32;

        // The Controls' fields, in the order EnginePlayer::Controls holds them and writeControls writes them.
        constexpr const char* kControls[] = {"ignition", "throttle", "brake", "clutch", "handbrake", "steer", "lever",
                                             "shiftUp", "shiftDown", "buttons", "mode", "dynoSpeed", "dynoRate",
                                             "holdSpeed", "strip", "scopeOn", "bakeMaps"};
        constexpr std::size_t kControlCount = sizeof(kControls) / sizeof(kControls[0]);

        // A control's value, by its index in kControls.
        double controlValue(const EnginePlayer::Controls& c, std::size_t k) noexcept
        {
            switch (k)
            {
                case 0: return c.ignition;
                case 1: return c.throttle;
                case 2: return c.brake;
                case 3: return c.clutch;
                case 4: return c.handbrake;
                case 5: return c.steer;
                case 6: return c.lever;
                case 7: return c.shiftUp;
                case 8: return c.shiftDown;
                case 9: return c.buttons;
                case 10: return c.mode;
                case 11: return c.dynoSpeed;
                case 12: return c.dynoRate;
                case 13: return c.holdSpeed;
                case 14: return c.strip;
                case 15: return c.scopeOn;
                case 16: return c.bakeMaps;
                default: return 0.0;
            }
        }

        int controlIndex(std::string_view name) noexcept
        {
            for (std::size_t k = 0; k < kControlCount; ++k)
                if (name == kControls[k])
                    return static_cast<int>(k);
            return -1;
        }

        void setControl(EnginePlayer::Controls& c, std::size_t k, double v) noexcept
        {
            const float f = static_cast<float>(v);
            const int i = static_cast<int>(std::lround(v));
            switch (k)
            {
                case 0: c.ignition = i; break;
                case 1: c.throttle = f; break;
                case 2: c.brake = f; break;
                case 3: c.clutch = f; break;
                case 4: c.handbrake = f; break;
                case 5: c.steer = f; break;
                case 6: c.lever = i; break;
                case 7: c.shiftUp = i; break;
                case 8: c.shiftDown = i; break;
                case 9: c.buttons = i; break;
                case 10: c.mode = i; break;
                case 11: c.dynoSpeed = f; break;
                case 12: c.dynoRate = f; break;
                case 13: c.holdSpeed = f; break;
                case 14: c.strip = i; break;
                case 15: c.scopeOn = i; break;
                case 16: c.bakeMaps = i; break;
                default: break;
            }
        }

        std::optional<std::string> readWhole(const std::filesystem::path& file)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in)
                return std::nullopt;
            std::ostringstream text;
            text << in.rdbuf();
            return text.str();
        }

        // A float as the shortest decimal that reads back as it: what a file shows of a value the Spec holds.
        double shortest(float v)
        {
            // (read back with strtod, not std::from_chars: libc++'s floating-point from_chars is "introduced in
            // macOS 26.0" and does not compile for our deployment target; the digits are C's, the locale "C")
            char text[32];
            const auto done = std::to_chars(text, text + sizeof text - 1, v);
            if (done.ec != std::errc())
                return static_cast<double>(v);
            *done.ptr = '\0';
            char* end = nullptr;
            const double out = std::strtod(text, &end);
            return end == done.ptr ? out : static_cast<double>(v);
        }
    } // namespace

    // ---- the player ------------------------------------------------------------------------------------------------

    std::unique_ptr<EnginePlayer> EnginePlayer::create(const Options& options, std::string& errors)
    {
        Program::Options compile;
        compile.optimisation = options.optimisation;
        compile.checks = options.checks;
        std::unique_ptr<EnginePlayer> player(new EnginePlayer());
        player->program_ = Program::fromDirectory(options.directory, options.entry, compile, errors);
        if (!player->program_)
            return nullptr;
        player->checks_ = options.checks;
        player->sampleRate_ = options.sampleRate > 0.0 ? options.sampleRate : 48000.0;
        if (!player->bind(errors))
            return nullptr;
        player->begin(false);
        return player;
    }

    EnginePlayer::~EnginePlayer()
    {
        end();
    }

    bool EnginePlayer::bind(std::string& errors)
    {
        fminus::Program& fm = program_->fminus();
        std::string error;
        schema_ = Schema::of(fm, "Engine", error);
        if (!schema_)
        {
            errors = "engine.fm: error: the contract wants a module Engine: " + error + "\n";
            return false;
        }
        if (!fm.nonBlocking("Engine::process"))
        {
            errors = "engine.fm: error: Engine::process is not marked [[clang::nonblocking]]\n";
            return false;
        }
        init_ = program_->function<void(void*, float)>("Engine::init", error);
        process_ = init_ ? program_->function<void(void*, float*, float*, int)>("Engine::process", error) : nullptr;
        end_ = process_ ? program_->function<void(void*)>("Engine::end", error) : nullptr;
        if (end_ == nullptr)
        {
            errors = "engine.fm: error: the contract wants Engine::init(float), Engine::process(left, right, n) and "
                     "Engine::end(): " + error + "\n";
            return false;
        }
        instance_ = std::make_unique<Instance>(*fm.module("Engine"));

        // the four blocks
        Schema::Handle spec, ctl, tel, scope;
        if (!schema_->find("spec", spec, error) || !schema_->find("ctl", ctl, error) || !schema_->find("tel", tel, error)
            || !schema_->find("scope", scope, error))
        {
            errors = "engine.fm: error: Engine wants the fields spec, ctl, tel and scope: " + error + "\n";
            return false;
        }
        specOffset_ = spec.offset;
        specSize_ = spec.node->size;

        // the Spec's numbers, and each section's rev
        specFields_.clear();
        telNames_.clear();
        telHandles_.clear();
        bool whole = true;
        schema_->forEach([&](const std::string& path, const Schema::Node& node, std::size_t offset) {
            if (path.rfind("spec.", 0) == 0)
            {
                const std::string sub = path.substr(5);
                const std::size_t dot = sub.find('.');
                if (dot == std::string::npos || node.name == "rev")
                    return;
                SpecField field;
                field.path = sub;
                field.section = sub.substr(0, dot);
                field.node = &node;
                field.offset = offset;
                field.count = node.count();
                Schema::Handle rev;
                std::string missing;
                if (!schema_->find("spec." + field.section + ".rev", rev, missing) || rev.scalar() != Scalar::Int)
                {
                    whole = false;
                    error = "spec." + field.section + " does not start with int rev";
                    return;
                }
                field.revOffset = rev.offset;
                if (const auto* range = node.attribute("fminus::range"); range != nullptr && range->arguments.size() == 2)
                {
                    field.hasRange = true;
                    field.minimum = range->arguments[0].number;
                    field.maximum = range->arguments[1].number;
                }
                if (const auto* def = node.attribute("fminus::default"); def != nullptr && !def->arguments.empty())
                    field.defaultValue = def->arguments[0].number;
                if (const auto* choices = node.attribute("ui::choices"); choices != nullptr)
                    for (const auto& argument : choices->arguments)
                        field.choices.push_back(argument.text);
                specFields_.push_back(std::move(field));
            }
            else if (path.rfind("tel.", 0) == 0 && node.extents.empty())
            {
                Schema::Handle handle;
                std::string ignored;
                if (schema_->find(path, handle, ignored))
                {
                    telNames_.push_back(path.substr(4));
                    telHandles_.push_back(handle);
                }
            }
        });
        if (!whole)
        {
            errors = "engine.fm: error: " + error + "\n";
            return false;
        }
        ctlHandles_.assign(kControlCount, Schema::Handle{});
        for (std::size_t k = 0; k < kControlCount; ++k)
        {
            if (!schema_->find(std::string("ctl.") + kControls[k], ctlHandles_[k], error)
                || ctlHandles_[k].scalar() == Scalar::None)
            {
                errors = "engine.fm: error: Controls is not the contract's: " + error + "\n";
                return false;
            }
        }
        budgetPerSample_ = static_cast<std::int64_t>(kLoopsPerSecond / sampleRate_);
        return true;
    }

    void EnginePlayer::begin(bool keepSpec)
    {
        fminus::Program& fm = program_->fminus();
        if (keepSpec)
            savedSpec_.assign(static_cast<const unsigned char*>(instance_->data()) + specOffset_,
                              static_cast<const unsigned char*>(instance_->data()) + specOffset_ + specSize_);
        instance_->reset();
        if (keepSpec)
        {
            std::memcpy(static_cast<unsigned char*>(instance_->data()) + specOffset_, savedSpec_.data(), specSize_);
            for (const SpecField& field : specFields_)
            {
                const Field<std::int32_t> rev{field.revOffset, 1};
                instance_->set(rev, instance_->get(rev) + 1);
            }
        }
        else
            setSpecDefaults();
        controls_ = Controls{};
        writeControls();
        samples_ = 0;
        stopped_ = false;
        stopReason_.clear();
        budgetPerSample_ = static_cast<std::int64_t>(kLoopsPerSecond / sampleRate_);

        fm.clearBoundsViolation();
        const ScopedFlushDenormals flush;
        fm.setLoopBudget(kInitBudget);
        init_(instance_->data(), static_cast<float>(sampleRate_));
        check("Engine::init");
        begun_ = true;
    }

    void EnginePlayer::end() noexcept
    {
        if (!begun_ || !program_)
            return;
        begun_ = false;
        fminus::Program& fm = program_->fminus();
        fm.clearBoundsViolation();
        const ScopedFlushDenormals flush;
        fm.setLoopBudget(kInitBudget);
        end_(instance_->data());
    }

    void EnginePlayer::reset(double sampleRate, bool keepSpec)
    {
        end();
        if (sampleRate > 0.0)
            sampleRate_ = sampleRate;
        begin(keepSpec);
    }

    int EnginePlayer::specIndex(std::string_view path) const noexcept
    {
        for (std::size_t k = 0; k < specFields_.size(); ++k)
            if (specFields_[k].path == path)
                return static_cast<int>(k);
        return -1;
    }

    void EnginePlayer::setSpec(std::size_t index, double value, std::size_t element) noexcept
    {
        const SpecField& field = specFields_[index];
        if (element >= field.count)
            return;
        double v = value;
        if (field.hasRange)
            v = std::clamp(v, std::min(field.minimum, field.maximum), std::max(field.minimum, field.maximum));
        if (field.isInt())
            v = static_cast<double>(std::lround(v));
        const Schema::Handle handle{field.node, field.offset, field.count};
        instance_->write(handle, v, element);
        const Field<std::int32_t> rev{field.revOffset, 1};
        instance_->set(rev, instance_->get(rev) + 1);
    }

    bool EnginePlayer::setSpec(std::string_view path, double value, std::string& error)
    {
        const int index = specIndex(path);
        if (index < 0)
        {
            error = "the Spec has no field '" + std::string(path) + "'";
            return false;
        }
        setSpec(static_cast<std::size_t>(index), value);
        return true;
    }

    double EnginePlayer::spec(std::size_t index, std::size_t element) const noexcept
    {
        const SpecField& field = specFields_[index];
        return instance_->read(Schema::Handle{field.node, field.offset, field.count}, element);
    }

    void EnginePlayer::setSpecDefaults() noexcept
    {
        for (std::size_t k = 0; k < specFields_.size(); ++k)
            for (std::size_t e = 0; e < specFields_[k].count; ++e)
                setSpec(k, specFields_[k].defaultValue, e);
    }

    void EnginePlayer::writeControls() noexcept
    {
        for (std::size_t k = 0; k < kControlCount; ++k)
            instance_->write(ctlHandles_[k], controlValue(controls_, k));
    }

    void EnginePlayer::check(const char* call) noexcept
    {
        if (!checks_ || stopped_)
            return;
        fminus::Program& fm = program_->fminus();
        if (const auto failed = fm.boundsViolation())
        {
            stopped_ = true;
            stopReason_ = std::string(call) + " failed a bounds check: " + failed->message();
        }
        else if (fm.loopBudgetExceeded())
        {
            stopped_ = true;
            stopReason_ = std::string(call) + " ran past its loop budget";
        }
    }

    void EnginePlayer::process(float* left, float* right, int n) noexcept
    {
        if (n < 0)
            return;
        if (stopped_)
        {
            std::memset(left, 0, static_cast<std::size_t>(n) * sizeof(float));
            std::memset(right, 0, static_cast<std::size_t>(n) * sizeof(float));
            samples_ += n;
            return;
        }
        writeControls();
        if (checks_)
            program_->fminus().setLoopBudget(budgetPerSample_ * std::max(n, kBudgetFloor));
        process_(instance_->data(), left, right, n);
        check("Engine::process");
        if (stopped_)
        {
            std::memset(left, 0, static_cast<std::size_t>(n) * sizeof(float));
            std::memset(right, 0, static_cast<std::size_t>(n) * sizeof(float));
        }
        samples_ += n;
    }

    double EnginePlayer::telemetry(std::size_t index) const noexcept
    {
        return instance_->read(telHandles_[index]);
    }

    int EnginePlayer::telemetryIndex(std::string_view name) const noexcept
    {
        for (std::size_t k = 0; k < telNames_.size(); ++k)
            if (telNames_[k] == name)
                return static_cast<int>(k);
        return -1;
    }

    bool EnginePlayer::read(std::string_view path, double& value, std::string& error) const
    {
        return instance_->read(*schema_, path, value, error);
    }

    // ---- builds ----------------------------------------------------------------------------------------------------

    bool parseBuild(const EnginePlayer& player, std::string_view json, std::string_view fileName, Build& build,
                    std::string& error, std::string& warnings)
    {
        const std::string file(fileName);
        build = Build{};
        Json document = Json::parse(json, nullptr, false);
        if (document.is_discarded() || !document.is_object())
        {
            error = file + ": not JSON, or not an object";
            return false;
        }
        const auto format = document.find("format");
        if (format == document.end() || *format != "tee-build")
        {
            error = file + ": \"format\" is \"tee-build\"";
            return false;
        }
        const auto version = document.find("version");
        if (version == document.end() || !version->is_number_integer() || version->get<int>() != 1)
        {
            error = file + ": \"version\" is 1 (the only version there is)";
            return false;
        }
        if (const auto name = document.find("name"); name != document.end() && name->is_string())
            build.name = name->get<std::string>();
        if (const auto notes = document.find("notes"); notes != document.end() && notes->is_string())
            build.notes = notes->get<std::string>();
        const auto spec = document.find("spec");
        if (spec == document.end())
            return true;
        if (!spec->is_object())
        {
            error = file + ": \"spec\" is an object of sections";
            return false;
        }
        for (const auto& [section, fields] : spec->items())
        {
            if (!fields.is_object())
            {
                error = file + ": spec." + section + " is an object of fields";
                return false;
            }
            for (const auto& [name, value] : fields.items())
            {
                const std::string path = section + "." + name;
                const int index = player.specIndex(path);
                if (index < 0)
                {
                    build.unknown[path] = value.dump();
                    warnings += file + ": spec." + path + " is no field of this simulator's: kept, and written back\n";
                    continue;
                }
                const EnginePlayer::SpecField& field = player.specFields()[static_cast<std::size_t>(index)];
                std::vector<double> values;
                const auto one = [&](const Json& v, double& out) {
                    if (v.is_number())
                        out = v.get<double>();
                    else if (v.is_boolean())
                        out = v.get<bool>() ? 1.0 : 0.0;
                    else if (v.is_string() && !field.choices.empty())
                    {
                        const auto found = std::find(field.choices.begin(), field.choices.end(), v.get<std::string>());
                        if (found == field.choices.end())
                            return false;
                        out = field.minimum + static_cast<double>(found - field.choices.begin());
                    }
                    else
                        return false;
                    return true;
                };
                if (value.is_array())
                {
                    for (const Json& element : value)
                    {
                        double v = 0.0;
                        if (!one(element, v))
                        {
                            error = file + ": spec." + path + " holds something that is no value of it";
                            return false;
                        }
                        values.push_back(v);
                    }
                }
                else
                {
                    double v = 0.0;
                    if (!one(value, v))
                    {
                        error = file + ": spec." + path + " is no value of it ("
                                + (field.choices.empty() ? std::string("a number") : "a number, or one of its choices")
                                + ")";
                        return false;
                    }
                    values.push_back(v);
                }
                if (values.size() > field.count)
                {
                    error = file + ": spec." + path + " holds more than its " + std::to_string(field.count) + " elements";
                    return false;
                }
                build.values[path] = std::move(values);
            }
        }
        return true;
    }

    bool loadBuild(const EnginePlayer& player, const std::filesystem::path& file, Build& build, std::string& error,
                   std::string& warnings)
    {
        const std::optional<std::string> text = readWhole(file);
        if (!text)
        {
            error = file.generic_string() + ": cannot be read";
            return false;
        }
        return parseBuild(player, *text, file.filename().generic_string(), build, error, warnings);
    }

    std::string buildJson(const EnginePlayer& player, const Build& build)
    {
        Json document = Json::object();
        document["format"] = "tee-build";
        document["version"] = 1;
        document["name"] = build.name;
        document["notes"] = build.notes;
        Json spec = Json::object();
        for (const EnginePlayer::SpecField& field : player.specFields())
        {
            const auto found = build.values.find(field.path);
            std::vector<double> values(field.count, field.defaultValue);
            if (found != build.values.end())
                std::copy(found->second.begin(), found->second.end(), values.begin());
            const auto valueJson = [&](double v) -> Json {
                if (!field.choices.empty() && field.isInt())
                {
                    const long k = std::lround(v - field.minimum);
                    if (k >= 0 && k < static_cast<long>(field.choices.size()))
                        return field.choices[static_cast<std::size_t>(k)];
                }
                if (field.isBool())
                    return v != 0.0;
                if (field.isInt())
                    return std::lround(v);
                return v;
            };
            const std::string name = field.path.substr(field.section.size() + 1);
            if (field.count == 1)
                spec[field.section][name] = valueJson(values[0]);
            else
            {
                Json array = Json::array();
                for (double v : values)
                    array.push_back(valueJson(v));
                spec[field.section][name] = array;
            }
        }
        for (const auto& [path, text] : build.unknown)
        {
            const std::size_t dot = path.find('.');
            spec[path.substr(0, dot)][path.substr(dot + 1)] = Json::parse(text, nullptr, false);
        }
        document["spec"] = spec;
        return document.dump(2) + "\n";
    }

    void applyBuild(EnginePlayer& player, const Build& build)
    {
        player.setSpecDefaults();
        for (const auto& [path, values] : build.values)
        {
            const int index = player.specIndex(path);
            if (index < 0)
                continue;
            for (std::size_t e = 0; e < values.size(); ++e)
                player.setSpec(static_cast<std::size_t>(index), values[e], e);
        }
    }

    Build currentBuild(const EnginePlayer& player, std::string name, std::string notes)
    {
        Build build;
        build.name = std::move(name);
        build.notes = std::move(notes);
        for (std::size_t k = 0; k < player.specFields().size(); ++k)
        {
            const EnginePlayer::SpecField& field = player.specFields()[k];
            std::vector<double> values;
            for (std::size_t e = 0; e < field.count; ++e)
            {
                const double v = player.spec(k, e);
                values.push_back(field.node->scalar == Scalar::Float ? shortest(static_cast<float>(v)) : v);
            }
            build.values[field.path] = std::move(values);
        }
        return build;
    }

    // ---- engine scenarios --------------------------------------------------------------------------------------------

    bool parseEngineScenario(std::string_view json, std::string_view fileName, EngineScenario& scenario,
                             std::string& error)
    {
        const std::string file(fileName);
        scenario = EngineScenario{};
        Json document = Json::parse(json, nullptr, false);
        if (document.is_discarded() || !document.is_object())
        {
            error = file + ": not JSON, or not an object";
            return false;
        }
        const auto format = document.find("format");
        if (format == document.end() || *format != "tee-engine-scenario/1")
        {
            error = file + ": \"format\" is \"tee-engine-scenario/1\"";
            return false;
        }
        if (const auto name = document.find("name"); name != document.end() && name->is_string())
            scenario.name = name->get<std::string>();
        else
            scenario.name = std::filesystem::path(file).stem().generic_string();
        if (const auto d = document.find("description"); d != document.end() && d->is_string())
            scenario.description = d->get<std::string>();
        if (const auto b = document.find("build"); b != document.end() && b->is_string())
            scenario.build = b->get<std::string>();
        const auto seconds = document.find("seconds");
        if (seconds == document.end() || !seconds->is_number() || seconds->get<double>() <= 0.0)
        {
            error = file + ": \"seconds\" is how long the scenario lasts, a number above 0";
            return false;
        }
        scenario.seconds = seconds->get<double>();
        const auto events = document.find("events");
        if (events == document.end() || !events->is_array())
        {
            error = file + ": \"events\" is an array";
            return false;
        }
        std::vector<const Json*> ordered;
        for (const Json& event : *events)
        {
            const auto t = event.is_object() ? event.find("t") : event.end();
            if (!event.is_object() || t == event.end() || !t->is_number() || t->get<double>() < 0.0)
            {
                error = file + ": each event is an object with \"t\", its time in seconds from the start";
                return false;
            }
            ordered.push_back(&event);
        }
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const Json* a, const Json* b) { return (*a)["t"].get<double>() < (*b)["t"].get<double>(); });

        const EnginePlayer::Controls defaults;
        std::map<std::string, double> now;   // each control's last target, for ramps
        for (std::size_t k = 0; k < kControlCount; ++k)
            now[kControls[k]] = controlValue(defaults, k);
        for (const Json* raw : ordered)
        {
            const Json& event = *raw;
            const double t = event["t"].get<double>();
            const std::string at = file + ": the event at t = " + Json(t).dump();
            double ramp = 0.0;
            for (const auto& [key, value] : event.items())
            {
                if (key == "t" || key == "note")
                    continue;
                if (key == "ramp")
                {
                    if (!value.is_number() || value.get<double>() < 0.0)
                    {
                        error = at + ": \"ramp\" is a time in seconds";
                        return false;
                    }
                    ramp = value.get<double>();
                }
                else if (key != "ctl" && key != "spec" && key != "press")
                {
                    error = at + ": \"" + key + "\" is not something an event can hold (ctl, spec, press, ramp, note)";
                    return false;
                }
            }
            if (const auto spec = event.find("spec"); spec != event.end())
            {
                if (!spec->is_object())
                {
                    error = at + ": \"spec\" is an object of numbers by path (\"engine.layout\")";
                    return false;
                }
                for (const auto& [path, value] : spec->items())
                {
                    if (!value.is_number() && !value.is_boolean())
                    {
                        error = at + ": spec " + path + " is a number";
                        return false;
                    }
                    const double v = value.is_boolean() ? (value.get<bool>() ? 1.0 : 0.0) : value.get<double>();
                    scenario.events.push_back({t, EngineEvent::Kind::Spec, path, v});
                }
            }
            if (const auto ctl = event.find("ctl"); ctl != event.end())
            {
                if (!ctl->is_object())
                {
                    error = at + ": \"ctl\" is an object of numbers by name (\"throttle\")";
                    return false;
                }
                for (const auto& [name, value] : ctl->items())
                {
                    if (controlIndex(name) < 0 || !value.is_number())
                    {
                        error = at + ": ctl \"" + name + "\" is no control, or not a number (sim/spec/controls.fm)";
                        return false;
                    }
                    const double to = value.get<double>();
                    const double from = now[name];
                    const int steps = ramp > 0.0 ? std::max(1, static_cast<int>(std::ceil(ramp / kRampStep - 1.0e-9))) : 1;
                    for (int k = 1; k <= steps; ++k)
                    {
                        const double share = static_cast<double>(k) / steps;
                        scenario.events.push_back({ramp > 0.0 ? t + ramp * share : t, EngineEvent::Kind::Control, name,
                                                   from + (to - from) * share});
                    }
                    now[name] = to;
                }
            }
            if (const auto press = event.find("press"); press != event.end())
            {
                std::vector<std::string> names;
                if (press->is_string())
                    names.push_back(press->get<std::string>());
                else if (press->is_array())
                    for (const Json& n : *press)
                        names.push_back(n.is_string() ? n.get<std::string>() : std::string());
                for (const std::string& name : names)
                {
                    if (name != "shiftUp" && name != "shiftDown" && name != "strip" && name != "bakeMaps")
                    {
                        error = at + ": \"press\" names counters: shiftUp, shiftDown, strip, bakeMaps";
                        return false;
                    }
                    scenario.events.push_back({t, EngineEvent::Kind::Count, name, 1.0});
                }
            }
        }
        std::stable_sort(scenario.events.begin(), scenario.events.end(),
                         [](const EngineEvent& a, const EngineEvent& b) { return a.time < b.time; });
        return true;
    }

    bool loadEngineScenario(const std::filesystem::path& file, EngineScenario& scenario, std::string& error)
    {
        const std::optional<std::string> text = readWhole(file);
        if (!text)
        {
            error = file.generic_string() + ": cannot be read";
            return false;
        }
        return parseEngineScenario(*text, file.filename().generic_string(), scenario, error);
    }

    bool renderEngine(EnginePlayer& player, const Build& build, const EngineScenario& scenario,
                      const RenderOptions& options, RenderResult& result, std::string& error)
    {
        using Clock = std::chrono::steady_clock;
        result = RenderResult{};
        player.reset();
        applyBuild(player, build);
        const double rate = player.sampleRate();

        std::vector<std::int64_t> eventAt(scenario.events.size());
        std::vector<int> eventField(scenario.events.size(), -1);
        for (std::size_t e = 0; e < scenario.events.size(); ++e)
        {
            const EngineEvent& event = scenario.events[e];
            eventAt[e] = std::llround(event.time * rate);
            eventField[e] = event.kind == EngineEvent::Kind::Spec ? player.specIndex(event.path) : controlIndex(event.path);
            if (eventField[e] < 0)
            {
                error = "the scenario " + scenario.name + " sets '" + event.path + "', and the program has no such field";
                return false;
            }
        }

        const double seconds = options.seconds > 0.0 ? options.seconds : scenario.seconds;
        const std::int64_t total = std::max<std::int64_t>(std::llround(seconds * rate), 0);
        const int block = std::max(options.blockSize, 1);
        result.audio.sampleRate = rate;
        result.columns = player.telemetryNames();
        const std::size_t columns = result.columns.size();
        const int stateColumn = player.telemetryIndex("engine.state");
        std::vector<float> left, right;
        if (options.keepAudio)
        {
            result.audio.left.assign(static_cast<std::size_t>(total), 0.0f);
            result.audio.right.assign(static_cast<std::size_t>(total), 0.0f);
        }
        else
        {
            left.assign(static_cast<std::size_t>(block), 0.0f);
            right.assign(static_cast<std::size_t>(block), 0.0f);
        }

        std::int64_t at = 0;
        std::size_t next = 0;
        while (at < total)
        {
            while (next < scenario.events.size() && eventAt[next] <= at)
            {
                const EngineEvent& event = scenario.events[next];
                const auto field = static_cast<std::size_t>(eventField[next]);
                switch (event.kind)
                {
                    case EngineEvent::Kind::Spec:
                        player.setSpec(field, event.value);
                        break;
                    case EngineEvent::Kind::Control:
                        setControl(player.controls(), field, event.value);
                        break;
                    case EngineEvent::Kind::Count:
                        setControl(player.controls(), field, controlValue(player.controls(), field) + 1.0);
                        break;
                }
                ++next;
            }
            std::int64_t n = std::min<std::int64_t>(block, total - at);
            if (next < scenario.events.size())
                n = std::min(n, eventAt[next] - at);
            float* l = options.keepAudio ? result.audio.left.data() + at : left.data();
            float* r = options.keepAudio ? result.audio.right.data() + at : right.data();

            const Clock::time_point start = Clock::now();
            player.process(l, r, static_cast<int>(n));
            const double took = std::chrono::duration<double>(Clock::now() - start).count();
            ++result.calls;
            if (stateColumn >= 0 && player.telemetry(static_cast<std::size_t>(stateColumn)) != 0.0)
            {
                result.soundingFrames += n;
                result.soundingSeconds += took;
                result.worstCallShare = std::max(result.worstCallShare, took * rate / static_cast<double>(n));
            }
            for (std::int64_t i = 0; i < n; ++i)
                result.peak = std::max(result.peak, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
            at += n;
            result.rowTime.push_back(static_cast<double>(at) / rate);
            for (std::size_t c = 0; c < columns; ++c)
                result.rows.push_back(static_cast<float>(player.telemetry(c)));
            if (player.stopped())
            {
                result.stopped = true;
                result.stopReason = player.stopReason();
                break;
            }
        }
        result.frames = at;
        return true;
    }

    // ---- the twin ------------------------------------------------------------------------------------------------------

    namespace
    {
        // The reference's dials, and the Spec field each is made from as the adapter makes it (sim/README.md, "The
        // first Spec"): as is, or converted back to the dial's unit with the same float arithmetic (FMinus compiles
        // without contraction, as this file is)
        enum class Back
        {
            Same,
            Litres,       // m^3 to L: x 1000
            Fahrenheit,   // K to the reference's F: (K - 300) x 1.8 + 80.33
        };
        struct DialOfSpec
        {
            const char* dial;
            const char* spec;
            Back back;
        };
        constexpr DialOfSpec kDialsOfSpec[] = {
            {"engine", "engine.layout", Back::Same}, {"size", "engine.displacement", Back::Litres},
            {"idle", "engine.idleTrim", Back::Same}, {"redline", "engine.redlineTrim", Back::Same},
            {"cam", "valve.cam", Back::Same}, {"inertia", "engine.flywheel", Back::Same},
            {"load", "engine.benchLoad", Back::Same}, {"turbo", "boost.level", Back::Same},
            {"bov", "boost.blowOff", Back::Same}, {"headers", "exhaust.headers", Back::Same},
            {"pipe", "exhaust.pipe", Back::Same}, {"xpipe", "exhaust.crossover", Back::Same},
            {"muffler", "exhaust.muffler", Back::Same}, {"pops", "ecu.pops", Back::Same},
            {"rough", "engine.rough", Back::Same}, {"view", "sound.view", Back::Same},
            {"distance", "sound.distance", Back::Same}, {"intake", "sound.intake", Back::Same},
            {"mech", "sound.mechanical", Back::Same}, {"room", "sound.room", Back::Same},
            {"level", "sound.level", Back::Same}, {"timing", "ecu.timing", Back::Same},
            {"turbosize", "boost.size", Back::Same}, {"starter", "ecu.starter", Back::Same},
            {"mass", "car.mass", Back::Same}, {"aspiration", "boost.kind", Back::Same},
            {"fuel", "fuel.kind", Back::Same}, {"compression", "engine.compression", Back::Same},
            {"borestroke", "engine.boreStroke", Back::Same}, {"ambient", "env.temperature", Back::Fahrenheit}};
    } // namespace

    // Where the engine's state (0 off, 1 cranking, 2 running, 3 running down) is in Engine, for the twin
    constexpr const char* kStatePath = "ecu.state";

    bool playTwin(EnginePlayer& engine, RefPlayer& ref, const Build& build, const EngineScenario& scenario,
                  const TwinOptions& options, TwinResult& result, std::string& error)
    {
        using Clock = std::chrono::steady_clock;
        result = TwinResult{};
        const double rate = engine.sampleRate();
        engine.reset();
        applyBuild(engine, build);
        ref.reset(rate);
        Instance& shared = ref.shared();
        const Schema& sharedSchema = ref.sharedSchema();

        // what is read back from the simulator and written into the reference, resolved once
        const Schema& schema = engine.schema();
        const auto find = [&](const std::string& path, Schema::Handle& handle) {
            if (schema.find(path, handle, error))
                return true;
            error = "the twin wants " + path + " in Engine: " + error;
            return false;
        };
        std::vector<int> dialSpec(std::size(kDialsOfSpec));
        std::vector<int> dialIndex(std::size(kDialsOfSpec));
        for (std::size_t k = 0; k < std::size(kDialsOfSpec); ++k)
        {
            dialIndex[k] = ref.dialIndex(kDialsOfSpec[k].dial);
            dialSpec[k] = engine.specIndex(kDialsOfSpec[k].spec);
            if (dialIndex[k] < 0 || dialSpec[k] < 0)
            {
                error = std::string("the twin cannot map the dial ") + kDialsOfSpec[k].dial;
                return false;
            }
        }
        // the adapter's own state (sim/engine.fm): the start key, the paddles' pulses, holdSpeed's key; the engine's
        Schema::Handle heldHandle, stateHandle, upHandle, downHandle, ghostHandle;
        if (!find("held", heldHandle) || !find(kStatePath, stateHandle) || !find("upPulse", upHandle)
            || !find("downPulse", downHandle) || !find("ghost", ghostHandle))
            return false;
        const int holdDial = ref.dialIndex("hold");
        const int driveDial = ref.dialIndex("drive");
        Schema::Handle refState, refStall, refArmed;
        if (!sharedSchema.find("state", refState, error) || !sharedSchema.find("stallT", refStall, error)
            || !sharedSchema.find("offArmed", refArmed, error) || driveDial < 0)
            return false;

        std::vector<std::int64_t> eventAt(scenario.events.size());
        std::vector<int> eventField(scenario.events.size(), -1);
        for (std::size_t e = 0; e < scenario.events.size(); ++e)
        {
            const EngineEvent& event = scenario.events[e];
            eventAt[e] = std::llround(event.time * rate);
            eventField[e] = event.kind == EngineEvent::Kind::Spec ? engine.specIndex(event.path) : controlIndex(event.path);
            if (eventField[e] < 0)
            {
                error = "the scenario " + scenario.name + " sets '" + event.path + "', and the program has no such field";
                return false;
            }
        }
        const double seconds = options.seconds > 0.0 ? options.seconds : scenario.seconds;
        const std::int64_t total = std::max<std::int64_t>(std::llround(seconds * rate), 0);
        const int block = std::max(options.blockSize, 1);
        result.engine.sampleRate = rate;
        result.reference.sampleRate = rate;
        const std::size_t kept = options.keepAudio ? static_cast<std::size_t>(total) : static_cast<std::size_t>(block);
        result.engine.left.assign(kept, 0.0f);
        result.engine.right.assign(kept, 0.0f);
        result.reference.left.assign(kept, 0.0f);
        result.reference.right.assign(kept, 0.0f);

        Instance& in = engine.instance();
        float none[1] = {0.0f};
        std::int64_t at = 0;
        std::size_t next = 0;
        while (at < total)
        {
            while (next < scenario.events.size() && eventAt[next] <= at)
            {
                const EngineEvent& event = scenario.events[next];
                const auto field = static_cast<std::size_t>(eventField[next]);
                if (event.kind == EngineEvent::Kind::Spec)
                    engine.setSpec(field, event.value);
                else if (event.kind == EngineEvent::Kind::Control)
                    setControl(engine.controls(), field, event.value);
                else
                    setControl(engine.controls(), field, controlValue(engine.controls(), field) + 1.0);
                ++next;
            }
            // the simulator applies what it was given (no samples), and the reference is given the same
            const int heldBefore = static_cast<int>(in.read(heldHandle));
            const int stateBefore = static_cast<int>(in.read(stateHandle));
            engine.process(none, none, 0);
            for (std::size_t k = 0; k < std::size(kDialsOfSpec); ++k)
            {
                float v = static_cast<float>(engine.spec(static_cast<std::size_t>(dialSpec[k])));
                if (kDialsOfSpec[k].back == Back::Litres)
                    v = v * 1000.0f;
                else if (kDialsOfSpec[k].back == Back::Fahrenheit)
                    v = (v - 300.0f) * 1.8f + 80.33f;
                ref.setDial(static_cast<std::size_t>(dialIndex[k]), static_cast<double>(v));
            }
            ref.setDial(static_cast<std::size_t>(holdDial), 1.0e9);
            const EnginePlayer::Controls& c = engine.controls();
            const bool driven = c.mode == EnginePlayer::kModeDrive;
            const int truck = static_cast<int>(engine.spec(static_cast<std::size_t>(engine.specIndex("car.kind"))));
            const int keys = static_cast<int>(in.read(ghostHandle));
            ref.setDial(static_cast<std::size_t>(driveDial), driven ? (truck == 1 ? 2 : 1) : (keys != 0 ? 3 : 0));
            RefPlayer::Input& input = ref.input();
            input.throttle = driven ? c.throttle : 0.0f;
            input.modwheel = driven ? 0.0f : c.throttle;
            input.aftertouch = 0.0f;
            input.pitchbend = 0.0f;
            input.brake = c.brake;
            input.clutch = c.clutch;
            input.gear = c.lever;
            input.buttons = (in.read(upHandle) == 1.0 ? 16 : 0) + (in.read(downHandle) == 1.0 ? 32 : 0);
            // the key, and the adapter's own writes of Shared's state (sim/engine.fm, Engine::ignition)
            const auto poke = [&](int state) {
                shared.write(refState, static_cast<double>(state));
                if (state == 3)
                {
                    shared.write(refStall, 0.0);
                    shared.write(refArmed, 0.0);
                }
            };
            const int ignition = engine.controls().ignition;
            if (ignition == EnginePlayer::kIgnitionStart)
            {
                if (stateBefore == 0 || stateBefore == 3)
                {
                    if (heldBefore != 0)
                        ref.stop();
                    ref.start(36.0f, 1.0f);
                    ++result.keyDowns;
                }
            }
            else
            {
                int s = stateBefore;
                if (heldBefore != 0)
                {
                    ref.stop();
                    if (s == 1)
                    {
                        poke(0);
                        s = 0;
                    }
                }
                if (ignition == EnginePlayer::kIgnitionOff && (s == 1 || s == 2))
                    poke(3);
            }

            std::int64_t n = std::min<std::int64_t>(block, total - at);
            if (next < scenario.events.size())
                n = std::min(n, eventAt[next] - at);
            const std::size_t o = options.keepAudio ? static_cast<std::size_t>(at) : 0;
            const bool sounded = ref.sounding();
            const Clock::time_point t0 = Clock::now();
            engine.process(result.engine.left.data() + o, result.engine.right.data() + o, static_cast<int>(n));
            const Clock::time_point t1 = Clock::now();
            ref.process(result.reference.left.data() + o, result.reference.right.data() + o, static_cast<int>(n));
            const Clock::time_point t2 = Clock::now();
            if (sounded)
            {
                result.soundingFrames += n;
                result.engineSeconds += std::chrono::duration<double>(t1 - t0).count();
                result.referenceSeconds += std::chrono::duration<double>(t2 - t1).count();
            }
            at += n;
            if (engine.stopped() || ref.stopped())
            {
                error = "a check stopped " + std::string(engine.stopped() ? "the simulator: " + engine.stopReason()
                                                                           : "the reference: " + ref.stopReason());
                return false;
            }
        }
        result.frames = at;
        return true;
    }
} // namespace tee
