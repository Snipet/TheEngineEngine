// tests/ui/MockPlayer.h: a made-up tee::Player for the kit's tests: parameters of every kind, a few telemetry values,
// an output level, and the last ControlsFrame it was given. Nothing plays.
#pragma once

#include <tee/app/Sources.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tee::test
{
    class MockPlayer final : public tee::Player
    {
    public:
        MockPlayer()
        {
            const auto real = [this](const char* id, double lo, double hi, double def) {
                tee::ParamInfo p;
                p.id = id;
                p.label = id;
                p.min = lo;
                p.max = hi;
                p.def = def;
                add(p);
            };
            real("cutoff", 20.0, 20000.0, 1000.0);
            for (const char* id : { "resonance", "level", "attack", "decay", "sustain", "release", "glow", "tone", "width",
                                    "depth", "rate", "feedback", "mix", "spread", "padx", "pady" })
                real(id, 0.0, 1.0, 0.5);
            real("drive", 0.0, 10.0, 2.0);
            real("pan", -1.0, 1.0, 0.0);
            real("tilt", -1.0, 1.0, 0.0);
            tee::ParamInfo sync;
            sync.id = sync.label = "sync";
            sync.kind = tee::ParamInfo::Kind::toggle;
            add(sync);
            tee::ParamInfo hold = sync;
            hold.id = hold.label = "hold";
            add(hold);
            tee::ParamInfo wave;
            wave.id = wave.label = "wave";
            wave.kind = tee::ParamInfo::Kind::choice;
            wave.choices = { "SAW", "SQUARE", "TRI", "SINE" };
            wave.max = 3;
            add(wave);
            tee::ParamInfo mode = wave;
            mode.id = mode.label = "mode";
            mode.choices = { "ALPHA", "BETA", "GAMMA" };
            mode.max = 2;
            add(mode);
            tee::ParamInfo steps;
            steps.id = steps.label = "steps";
            steps.kind = tee::ParamInfo::Kind::integer;
            steps.min = 1;
            steps.max = 4;
            steps.def = 2;
            add(steps);
            telemetry_ = { { "showRpm", 850.0 }, { "gate", 0.0 }, { "boost", 0.4 }, { "showGear", 0.0 } };
        }

        // ParamSource
        int count() const override { return static_cast<int>(params_.size()); }
        const tee::ParamInfo& info(int i) const override { return params_[static_cast<std::size_t>(i)]; }
        double get(int i) const override { return values_[static_cast<std::size_t>(i)]; }
        void set(int i, double value) override
        {
            values_[static_cast<std::size_t>(i)] = value;
            ++sets;
        }
        // ParamSource's and TelemetrySource's at once: both declare find(std::string_view) const, so a Player has one
        // (the card's text as it stands). A parameter's index is 0..count()-1; a telemetry value's handle starts at
        // kTelemetry, so that neither is taken for the other.
        static constexpr int kTelemetry = 1000;
        int find(std::string_view id) const override
        {
            for (std::size_t i = 0; i < params_.size(); ++i)
                if (params_[i].id == id)
                    return static_cast<int>(i);
            for (std::size_t i = 0; i < telemetry_.size(); ++i)
                if (telemetry_[i].first == id)
                    return kTelemetry + static_cast<int>(i);
            return -1;
        }
        double plain(std::string_view id) const { return get(find(id)); }

        // TelemetrySource
        double value(int handle) const override
        {
            const int i = handle - kTelemetry;
            return i >= 0 && i < static_cast<int>(telemetry_.size()) ? telemetry_[static_cast<std::size_t>(i)].second : 0.0;
        }
        float outputPeak() const override { return peak; }
        void setTelemetry(std::string_view name, double v)
        {
            telemetry_[static_cast<std::size_t>(find(name) - kTelemetry)].second = v;
        }

        // ControlSink, AudioSource
        void setControls(const tee::ControlsFrame& f) override
        {
            controls = f;
            ++frames;
        }
        void prepare(double, int) override {}
        void process(float* l, float* r, int n) noexcept override
        {
            std::fill(l, l + n, 0.0f);
            std::fill(r, r + n, 0.0f);
        }

        float peak = 0.25f;
        int sets = 0, frames = 0;
        tee::ControlsFrame controls{};

    private:
        void add(const tee::ParamInfo& p)
        {
            params_.push_back(p);
            values_.push_back(p.def);
        }

        std::vector<tee::ParamInfo> params_;
        std::vector<double> values_;
        std::vector<std::pair<std::string, double>> telemetry_;
    };
}
