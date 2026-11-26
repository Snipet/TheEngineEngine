// shell/input/g29/RacingWheelLinux.cpp: RacingWheel.h on Linux. The wheel is the kernel's hid-lg4ff's there, read and
// commanded through evdev and sysfs by g29/G29Evdev.h's Hub (which g29/G29Driver.h decides for, as on macOS). Ported
// from TestingPlayground's Source/plugin/standalone/RacingWheelLinux.cpp (47bac40; g29/SEED.tsv), whose event loop was
// JUCE's: here it is pump(), once a frame on the main thread: the nodes' descriptors and an inotify watch of
// /dev/input (a wheel plugged in or out, a udev rule's permissions arriving) polled without waiting, and the driver's
// two timers as deadlines on the steady clock. A frame's lateness is a timer's (at most a frame: 16 ms against the
// lights' 25 and the switch's 8 s). Main thread only.
#if defined(_WIN32)
#error "Linux only: shell/CMakeLists.txt builds it there (Windows: RacingWheelNone.cpp)"
#endif

#include "input/g29/RacingWheel.h"

#include "input/g29/G29Evdev.h"

#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace tee::input::g29
{
    namespace
    {
        using Clock = std::chrono::steady_clock;
        constexpr double kRescanSeconds = 0.150;             // after /dev/input changes: udev's permissions settle

        double seconds() { return std::chrono::duration<double>(Clock::now().time_since_epoch()).count(); }
    }

    struct RacingWheel::Impl final : evdev::System
    {
        Hooks hooks;
        double due[2] = { -1.0, -1.0 };                      // Timer's deadlines (seconds); negative: disarmed
        double rescanDue = -1.0;
        std::set<int> fds;                                   // the nodes watched
        int notify = -1;                                     // inotify on /dev/input
        std::unique_ptr<evdev::Hub> hub;                     // last: it goes first, while the rest is here

        Impl(Hooks h, bool on) : hooks(std::move(h))
        {
            hub = std::make_unique<evdev::Hub>(*this, evdev::Paths{}, on);
            notify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
            if (notify >= 0
                && inotify_add_watch(notify, "/dev/input", IN_CREATE | IN_DELETE | IN_ATTRIB | IN_MOVED_TO | IN_MOVED_FROM) < 0)
            {
                ::close(notify);
                notify = -1;
            }
            hub->rescan();
        }

        ~Impl() override
        {
            hub.reset();                                     // the driver quits: the spring and the lights off
            if (notify >= 0)
                ::close(notify);
        }

        // Once a frame: what the nodes and /dev/input say, then the timers that are due.
        void pump()
        {
            std::vector<pollfd> watch;
            for (const int fd : fds)
                watch.push_back({ fd, POLLIN, 0 });
            if (notify >= 0)
                watch.push_back({ notify, POLLIN, 0 });
            if (!watch.empty() && ::poll(watch.data(), watch.size(), 0) > 0)
                for (const pollfd& p : watch)
                {
                    if ((p.revents & (POLLIN | POLLHUP | POLLERR)) == 0)
                        continue;
                    if (p.fd == notify)
                    {
                        alignas(inotify_event) char buf[4096];
                        while (::read(notify, buf, sizeof buf) > 0)
                        {
                        }
                        rescanDue = seconds() + kRescanSeconds;
                    }
                    else if (hub != nullptr && fds.count(p.fd) != 0)
                        hub->readable(p.fd);
                }
            const double now = seconds();
            if (rescanDue >= 0.0 && now >= rescanDue)
            {
                rescanDue = -1.0;
                if (hub != nullptr)
                    hub->rescan();
            }
            for (const Timer t : { Timer::switched, Timer::lights })
            {
                double& d = due[static_cast<int>(t)];
                if (d >= 0.0 && now >= d)
                {
                    d = -1.0;
                    if (hub != nullptr)
                        hub->driver().fire(t);
                }
            }
        }

        // ---- evdev::System -------------------------------------------------------------------------------------------
        void watch(int fd) override { fds.insert(fd); }
        void unwatch(int fd) override { fds.erase(fd); }
        void arm(Timer t, double s) override { due[static_cast<int>(t)] = seconds() + std::max(0.0, s); }
        void disarm(Timer t) override { due[static_cast<int>(t)] = -1.0; }
        void changed() override
        {
            if (hooks.changed)
                hooks.changed();
        }
        int lights() override { return hooks.lights ? hooks.lights() : -1; }
    };

    RacingWheel::RacingWheel(Hooks hooks, bool on) : impl_(std::make_unique<Impl>(std::move(hooks), on)) {}
    RacingWheel::~RacingWheel() = default;

    bool RacingWheel::supported() noexcept { return true; }
    bool RacingWheel::present() const noexcept { return impl_->hub->driver().present(); }
    bool RacingWheel::on() const noexcept { return impl_->hub->driver().on(); }
    void RacingWheel::setOn(bool on) { impl_->hub->driver().setOn(on); }
    bool RacingWheel::reading() const noexcept { return impl_->hub->driver().reading(); }
    State RacingWheel::state() const { return impl_->hub->driver().state(); }
    Presses RacingWheel::takePresses() { return impl_->hub->driver().takePresses(); }
    std::string RacingWheel::status() const { return impl_->hub->driver().status(); }
    void RacingWheel::pump() { impl_->pump(); }
}
