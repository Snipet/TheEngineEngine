// shell/player/SilentPlayer.h: the Player the application holds when the simulator did not compile: no parameters,
// no telemetry, silence. The window stays up around it and shows the compiler's words.
#pragma once

#include "tee/app/Sources.h"

#include <algorithm>

namespace tee::player
{
    class SilentPlayer final : public Player
    {
    public:
        int count() const override { return 0; }
        const ParamInfo& info(int) const override { return none_; }
        double get(int) const override { return 0.0; }
        void set(int, double) override {}
        int findParam(std::string_view) const override { return -1; }
        int findValue(std::string_view) const override { return -1; }
        double value(int) const override { return 0.0; }
        float outputPeak() const override { return 0.0f; }
        int outputTail(float*, int) const override { return 0; }
        void setControls(const ControlsFrame&) override {}
        void prepare(double, int) override {}
        void process(float* l, float* r, int n) noexcept override
        {
            std::fill(l, l + n, 0.0f);
            std::fill(r, r + n, 0.0f);
        }

    private:
        ParamInfo none_;
    };
}
