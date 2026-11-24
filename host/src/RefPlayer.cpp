// host/src/RefPlayer.cpp: tee::RefPlayer (RefPlayer.h).
#include "tee/RefPlayer.h"

#include "tee/Denormals.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace tee
{
    namespace
    {
        // The functions the player calls the Voice's audio-thread methods through, compiled after program.fm's last
        // line. Each is marked nonblocking, so FMinus refuses the program if the method it calls could reach malloc,
        // calloc or free; each is an entry point, so it sets flush-to-zero when called and restores the caller's mode.
        const char* const kShims =
            "void tee_ref_start(Voice* self, float note, float velocity) [[clang::nonblocking]] "
            "{ self->start(note, velocity); }\n"
            "bool tee_ref_process(Voice* self, float* restrict left, float* restrict right, int n) "
            "[[clang::nonblocking]] { return self->process(left, right, n); }\n"
            "void tee_ref_stop(Voice* self) [[clang::nonblocking]] { self->stop(); }\n";

        // The loop budget, as the synth sets it: 2^32 loop iterations for a second of samples (about what a core
        // runs when an iteration takes a cycle), 16 samples' worth at least for a call, and 2^32 for an init or end.
        constexpr double kLoopsPerSecond = 4294967296.0;
        constexpr int kBudgetFloor = 16;
        constexpr std::int64_t kInitBudget = std::int64_t{1} << 32;

        bool hasMethod(const fminus::Program::Module& module, std::string_view qualified)
        {
            return std::find(module.methods.begin(), module.methods.end(), qualified) != module.methods.end();
        }
    } // namespace

    std::unique_ptr<RefPlayer> RefPlayer::create(const Options& options, std::string& errors)
    {
        Program::Options compile;
        compile.optimisation = options.optimisation;
        compile.checks = options.checks;
        compile.appended = kShims;

        std::unique_ptr<RefPlayer> player(new RefPlayer());
        player->program_ = Program::fromDirectory(options.directory, "program.fm", compile, errors);
        if (!player->program_)
            return nullptr;
        player->checks_ = options.checks;
        player->sampleRate_ = options.sampleRate > 0.0 ? options.sampleRate : 48000.0;
        if (!player->bind(errors))
            return nullptr;
        player->begin();
        return player;
    }

    RefPlayer::~RefPlayer()
    {
        end();
    }

    // Everything of the contract found in the compiled program: the three modules, their instances, the methods, the
    // Host's fields, the dials and the show fields. False, with `errors` saying what is missing.
    bool RefPlayer::bind(std::string& errors)
    {
        fminus::Program& fm = program_->fminus();
        std::string error;
        sharedSchema_ = Schema::of(fm, "Shared", error);
        voiceSchema_ = sharedSchema_ ? Schema::of(fm, "Voice", error) : nullptr;
        hostSchema_ = voiceSchema_ ? Schema::of(fm, "Host", error) : nullptr;
        if (!hostSchema_)
        {
            errors = "program.fm: error: the reference's contract wants modules Shared, Voice and Host: " + error + "\n";
            return false;
        }
        const fminus::Program::Module& sharedModule = *fm.module("Shared");
        const fminus::Program::Module& voiceModule = *fm.module("Voice");
        shared_ = std::make_unique<Instance>(sharedModule);
        voice_ = std::make_unique<Instance>(voiceModule);
        host_ = std::make_unique<Instance>(*fm.module("Host"));

        // The methods. A wrong signature here would be undefined behaviour, so the shims' types (which FMinus checked
        // against the methods when it compiled them) are the ones called on the audio thread.
        sharedInit_ = program_->function<void(void*, float)>("Shared::init", error);
        sharedEnd_ = sharedInit_ ? program_->function<void(void*)>("Shared::end", error) : nullptr;
        start_ = sharedEnd_ ? program_->function<void(void*, float, float)>("tee_ref_start", error) : nullptr;
        process_ = start_ ? program_->function<bool(void*, float*, float*, int)>("tee_ref_process", error) : nullptr;
        stop_ = process_ ? program_->function<void(void*)>("tee_ref_stop", error) : nullptr;
        if (stop_ == nullptr)
        {
            errors = "program.fm: error: the reference's contract wants Shared::init(float), Shared::end() and the "
                     "Voice's start, process and stop: " + error + "\n";
            return false;
        }
        if (hasMethod(voiceModule, "Voice::init"))
            voiceInit_ = program_->function<void(void*, float)>("Voice::init", error);
        if (hasMethod(voiceModule, "Voice::end"))
            voiceEnd_ = program_->function<void(void*)>("Voice::end", error);

        // Host's fields, each by name and type.
        HostFields& h = hostFields_;
        const auto f = [&](const char* name) { return hostSchema_->field<float>(name, error); };
        const auto i = [&](const char* name) { return hostSchema_->field<std::int32_t>(name, error); };
        const auto b = [&](const char* name) { return hostSchema_->field<bool>(name, error); };
        h.bpm = f("bpm");
        h.beat = f("beat");
        h.barStart = f("barStart");
        h.modwheel = f("modwheel");
        h.pitchbend = f("pitchbend");
        h.aftertouch = f("aftertouch");
        h.steering = f("steering");
        h.throttle = f("throttle");
        h.brake = f("brake");
        h.clutch = f("clutch");
        h.beatsPerBar = i("beatsPerBar");
        h.beatUnit = i("beatUnit");
        h.gear = i("gear");
        h.buttons = i("buttons");
        h.playing = b("playing");
        h.sustain = b("sustain");
        const bool hostWhole = h.bpm && h.beat && h.barStart && h.modwheel && h.pitchbend && h.aftertouch && h.steering
                               && h.throttle && h.brake && h.clutch && h.beatsPerBar && h.beatUnit && h.gear
                               && h.buttons && h.playing && h.sustain;
        if (!hostWhole)
        {
            errors = "host.fm: error: module Host is not the one the reference was written for: " + error + "\n";
            return false;
        }

        // The dials: the Voice's parameters, in declaration order.
        dials_.clear();
        dialParameters_.clear();
        for (const fminus::Program::Parameter& parameter : voiceModule.parameters)
        {
            Dial dial;
            dial.name = parameter.name;
            dial.type = parameter.type;
            dial.minimum = parameter.minimum.value_or(0.0);
            dial.maximum = parameter.maximum.value_or(0.0);
            dial.defaultValue = parameter.defaultValue.value_or(0.0);
            dials_.push_back(std::move(dial));
            dialParameters_.push_back(&parameter);
        }

        // What the panel shows: Shared's own float fields named show*.
        showNames_.clear();
        showFields_.clear();
        for (const Schema::Node& node : sharedSchema_->fields())
        {
            if (node.name.rfind("show", 0) == 0 && node.scalar == Scalar::Float && node.extents.empty())
            {
                showNames_.push_back(node.name);
                showFields_.push_back(Field<float>{node.offset, 1});
            }
        }
        budgetPerSample_ = static_cast<std::int64_t>(kLoopsPerSecond / sampleRate_);
        return true;
    }

    // Makes the instances ready and runs the inits: Shared's before the Voice's.
    void RefPlayer::begin()
    {
        fminus::Program& fm = program_->fminus();
        shared_->reset();
        voice_->reset();
        host_->reset();
        // Every field of type Shared* or Host*, of the Voice and of Shared, points at the one of each.
        const auto point = [&](Instance& instance, const Schema& schema) {
            for (const Schema::Node& node : schema.fields())
            {
                if (node.scalar != Scalar::Pointer || !node.extents.empty())
                    continue;
                void* target = node.type == "Shared *" ? shared_->data() : (node.type == "Host *" ? host_->data() : nullptr);
                if (target != nullptr)
                    instance.set(Field<void*>{node.offset, 1}, target);
            }
        };
        point(*voice_, *voiceSchema_);
        point(*shared_, *sharedSchema_);

        input_ = Input{};
        samples_ = 0;
        sounding_ = false;
        stopped_ = false;
        stopReason_.clear();
        budgetPerSample_ = static_cast<std::int64_t>(kLoopsPerSecond / sampleRate_);
        writeHost();

        // The record of a failed bounds check is emptied first: while it holds one, every call returns partway.
        fm.clearBoundsViolation();
        const ScopedFlushDenormals flush;   // init runs in the mode the program is compiled for
        fm.setLoopBudget(kInitBudget);
        sharedInit_(shared_->data(), static_cast<float>(sampleRate_));
        check("Shared's init()");
        if (voiceInit_ != nullptr && !stopped_)
        {
            fm.setLoopBudget(kInitBudget);
            voiceInit_(voice_->data(), static_cast<float>(sampleRate_));
            check("the Voice's init()");
        }
        begun_ = true;
    }

    // The ends, the Voice's before Shared's: what init took is given back.
    void RefPlayer::end() noexcept
    {
        if (!begun_ || !program_)
            return;
        begun_ = false;
        fminus::Program& fm = program_->fminus();
        fm.clearBoundsViolation();
        const ScopedFlushDenormals flush;
        if (voiceEnd_ != nullptr)
        {
            fm.setLoopBudget(kInitBudget);
            voiceEnd_(voice_->data());
        }
        fm.setLoopBudget(kInitBudget);
        sharedEnd_(shared_->data());
    }

    void RefPlayer::reset(double sampleRate)
    {
        end();
        if (sampleRate > 0.0)
            sampleRate_ = sampleRate;
        begin();
    }

    int RefPlayer::dialIndex(std::string_view name) const noexcept
    {
        for (std::size_t k = 0; k < dials_.size(); ++k)
            if (dials_[k].name == name)
                return static_cast<int>(k);
        return -1;
    }

    bool RefPlayer::setDial(std::string_view name, double value) noexcept
    {
        const int index = dialIndex(name);
        if (index < 0)
            return false;
        setDial(static_cast<std::size_t>(index), value);
        return true;
    }

    void RefPlayer::setDial(std::size_t index, double value) noexcept
    {
        fminus::Program::setValue(voice_->data(), *dialParameters_[index], value);
    }

    double RefPlayer::dial(std::size_t index) const noexcept
    {
        return fminus::Program::value(voice_->data(), *dialParameters_[index]);
    }

    void RefPlayer::setDefaults() noexcept
    {
        fminus::Program::setDefaults(voice_->data(), voice_->module());
    }

    int RefPlayer::showIndex(std::string_view name) const noexcept
    {
        for (std::size_t k = 0; k < showNames_.size(); ++k)
            if (showNames_[k] == name)
                return static_cast<int>(k);
        return -1;
    }

    // Host, as the calls read it: the player's input, and a transport that plays at 120 beats a minute from the
    // program's start (what the synth's Standalone says: the reference reads none of it).
    void RefPlayer::writeHost() noexcept
    {
        const HostFields& h = hostFields_;
        Instance& host = *host_;
        const double beat = static_cast<double>(samples_) * 120.0 / (60.0 * sampleRate_);
        host.set(h.bpm, 120.0f);
        host.set(h.beat, static_cast<float>(beat));
        host.set(h.barStart, static_cast<float>(std::floor(beat / 4.0) * 4.0));
        host.set(h.beatsPerBar, std::int32_t{4});
        host.set(h.beatUnit, std::int32_t{4});
        host.set(h.playing, true);
        host.set(h.modwheel, input_.modwheel);
        host.set(h.pitchbend, input_.pitchbend);
        host.set(h.aftertouch, input_.aftertouch);
        host.set(h.sustain, input_.sustain);
        host.set(h.steering, input_.steering);
        host.set(h.throttle, input_.throttle);
        host.set(h.brake, input_.brake);
        host.set(h.clutch, input_.clutch);
        host.set(h.gear, static_cast<std::int32_t>(input_.gear));
        host.set(h.buttons, static_cast<std::int32_t>(input_.buttons));
    }

    void RefPlayer::budget(int samples) noexcept
    {
        if (checks_)
            program_->fminus().setLoopBudget(budgetPerSample_ * std::max(samples, kBudgetFloor));
    }

    // After a call of a program compiled with the checks: whether it ran out of its budget or failed a bounds check.
    // The message allocates: this path is taken once, and the program plays no more after it.
    void RefPlayer::check(const char* call) noexcept
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
        if (stopped_)
            sounding_ = false;
    }

    void RefPlayer::start(float note, float velocity) noexcept
    {
        if (stopped_)
            return;
        writeHost();
        budget(kBudgetFloor);
        start_(voice_->data(), note, velocity);
        sounding_ = true;
        check("start()");
    }

    void RefPlayer::stop() noexcept
    {
        if (stopped_)
            return;
        writeHost();
        budget(kBudgetFloor);
        stop_(voice_->data());
        check("stop()");
    }

    bool RefPlayer::process(float* left, float* right, int n) noexcept
    {
        if (n <= 0)
            return sounding_;
        std::memset(left, 0, static_cast<std::size_t>(n) * sizeof(float));
        std::memset(right, 0, static_cast<std::size_t>(n) * sizeof(float));
        if (sounding_ && !stopped_)
        {
            writeHost();
            budget(n);
            sounding_ = process_(voice_->data(), left, right, n);
            check("process()");
            if (stopped_)
            {
                // what a stopped call left in the buffers is no sound of the program's
                std::memset(left, 0, static_cast<std::size_t>(n) * sizeof(float));
                std::memset(right, 0, static_cast<std::size_t>(n) * sizeof(float));
            }
        }
        samples_ += n;
        return sounding_;
    }
} // namespace tee
