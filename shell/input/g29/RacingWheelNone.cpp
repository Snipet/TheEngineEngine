// shell/input/g29/RacingWheelNone.cpp: RacingWheel.h where the application reads no racing wheel yet (Windows, Linux:
// docs/controls.md). None is ever present.
#include "input/g29/RacingWheel.h"

namespace tee::input::g29
{
    struct RacingWheel::Impl
    {
        bool on = false;
    };

    RacingWheel::RacingWheel(Hooks, bool on) : impl_(std::make_unique<Impl>())
    {
        impl_->on = on;
    }

    RacingWheel::~RacingWheel() = default;

    bool RacingWheel::supported() noexcept { return false; }
    bool RacingWheel::present() const noexcept { return false; }
    bool RacingWheel::on() const noexcept { return impl_->on; }
    void RacingWheel::setOn(bool on) { impl_->on = on; }
    bool RacingWheel::reading() const noexcept { return false; }
    State RacingWheel::state() const { return {}; }
    Presses RacingWheel::takePresses() { return {}; }
    std::string RacingWheel::status() const { return {}; }
    void RacingWheel::pump() {}
}
