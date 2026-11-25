// shell/input/g29/G29Driver.h: the G29 as decisions, apart from IOKit: which G29 is read, when a compatibility-mode one
// is switched to its own mode, its range and spring, what is taken from the device, its shift lights, and the status
// line. RacingWheel (g29/RacingWheel.h) is its transport over IOKit's HID manager; tests/shell/G29Test.cpp drives it
// with fake devices that record what they are sent. IOKit-free. Ported from TestingPlayground's
// Source/plugin/WheelDriver.{h,cpp} (47bac40; SEED.tsv), where it drove the owner's G29 on a Mac; the MIDI is taken
// out (state() is read a frame at a time, so there is no flush and no rate), the rules are as they were:
//
//   - Read: a G29 in its own mode (Kind::g29) while it is on: 900 degrees (kRange), then the spring (kSpring), and no
//     spring when the range fails (the status says so); its values through a Map.
//   - The seed: what a device holds before any report came is IOKit's cache at 0 (its timestamp 0), not the wheel's
//     state (a pedal at 0 is pressed, the steering full left, the hat up): such a value is never taken, so the state
//     is at rest for it. A value is taken only from a report (its timestamp set).
//   - The switch: a compatibility-mode G29 (Kind::compat) gets hid-lg4ff's two reports at most once each time it is
//     switched on (a wheel that comes back in that mode is not switched again until then, so it cannot loop). It leaves
//     and comes back as a G29; while it is away the switch is pending ("switching to its full mode"); after
//     kSwitchSeconds without it, or when it comes back in its compatibility mode, the switch failed and the status says
//     what to do.
//   - The spring stops (kSpringOff) on every way out that leaves a device to send it to: switched off, another G29 read
//     in its place, quit(); a report that fails says so (its spring may still be on). The wheel unplugged takes its
//     spring with it. A crash or a force-quit sends nothing: the spring stays until it is unplugged.
//   - Reading ends (switched off, the wheel gone): the state goes back to rest (Map::rest), so nothing stays held.
//   - The shift lights: while a G29 is read, every kLightsInterval, the mask the application asks for
//     (Transport::lights: the DRIVE view's count) goes to the wheel when it differs from what was sent, so at most 40
//     reports a second and none while it holds; none asked turns off what was lit. Reading starts with none lit.
//     Reading ends (switched off, another G29 read in its place, quit()): off, when any are lit; unplugged, they go
//     with it. A report that fails says so and is not tried again until the mask changes.
//
// One thread (the application's main thread). Transport::changed is called whenever present() or status() may have
// changed.
#pragma once

#include "input/g29/G29.h"

#include <cstdint>
#include <string>
#include <vector>

namespace tee::input::g29
{
    using Device = const void*;                              // the transport's (an IOHIDDeviceRef), never followed here

    // An input element's value as the device last reported it: `timestamp` 0 when no report has carried it yet.
    struct Value
    {
        std::uint32_t page = 0, usage = 0;
        std::int64_t value = 0, min = 0, max = 0;            // its logical range, as the descriptor has it
        std::uint32_t reportSize = 0;
        std::uint64_t timestamp = 0;
    };

    enum class Timer : std::uint8_t { switched, lights };

    class Transport
    {
    public:
        virtual ~Transport() = default;
        virtual int report(Device d, const Report& r) = 0;  // an output report: 0 when it went, else its error (IOReturn)
        virtual int setRange(Device d, int degrees) { return report(d, rangeReport(degrees)); }
        // The spring at `magnitude` (0: off, kSpringOff), its reports in order, stopping at the first that fails.
        virtual int setSpring(Device d, std::uint16_t magnitude)
        {
            if (magnitude == 0)
                return report(d, kSpringOff);
            for (const Report& r : springReports(magnitude))
                if (const int e = report(d, r); e != 0)
                    return e;
            return 0;
        }
        virtual int setLights(Device d, int mask) { return report(d, lightsReport(mask)); }
        // The switch to its own mode (kToNative): the first report's error; the second may fail as the wheel detaches.
        virtual int toNative(Device d)
        {
            if (const int e = report(d, kToNative[0]); e != 0)
                return e;
            report(d, kToNative[1]);
            return 0;
        }
        virtual std::string describe(int error);            // an error in words ("0x...", IOKit's code, by default)
        virtual std::vector<Value> values(Device d) = 0;    // its input elements' values now (the seed)
        virtual void listen(Device d) = 0;                  // its values to Driver::value from now on (nullptr: none's)
        virtual void arm(Timer t, double seconds) = 0;      // Driver::fire(t) once, `seconds` from now (armed again: moved)
        virtual void disarm(Timer t) = 0;
        virtual void changed() = 0;
        virtual int lights() = 0;                           // the shift lights asked for now: their mask, -1 none
    };

    class Driver
    {
    public:
        static constexpr double kSwitchSeconds = 8.0;       // the switch's re-enumeration, at the most
        static constexpr double kLightsInterval = 0.025;    // the shift lights looked at, and sent when they changed
        static constexpr int kNotPermitted = static_cast<int>(0xE00002E2u);   // kIOReturnNotPermitted

        // The transport is the caller's, alive until quit(); the constructor calls none of it.
        Driver(Transport& transport, bool on) : t_(transport), on_(on) {}
        Driver(const Driver&) = delete;
        Driver& operator=(const Driver&) = delete;

        void opened(int result);                            // IOHIDManagerOpen's (0: open)
        void added(Device d, Kind kind);                    // a device matched (Kind::none: left alone)
        void removed(Device d);
        void value(Device d, const Value& v);               // one of the device's values, from a report
        void fire(Timer t);
        void setOn(bool on);
        // The application quits: the spring and the lights off, nothing done after it. The owner calls it before the
        // transport goes (the destructor calls nothing).
        void quit();

        bool on() const noexcept { return on_; }
        bool present() const noexcept;                      // a G29 connected, in any mode, or a switch's news
        bool reading() const noexcept { return reading_ != nullptr; }
        const State& state() const noexcept { return map_.state(); }   // at rest while not reading
        std::string status() const;                         // a short line, "" when there is none

    private:
        struct Known
        {
            Device d;
            Kind kind;
        };
        enum class Switch : std::uint8_t { none, pending, failed };

        const Known* find(Kind kind) const noexcept;
        bool has(Device d) const noexcept;
        bool take(const Value& v);
        void startReading(Device d);
        void stopReading(bool stillThere);
        void switchToNative(Device d);
        void pollLights();
        void lightsOff();
        void endSwitch(Switch to);
        void refresh();

        Transport& t_;
        bool on_ = false, quit_ = false;
        int opened_ = 0;
        std::vector<Known> devices_;
        Device reading_ = nullptr;                          // the G29 set up and read, while on
        Map map_;
        bool switchUsed_ = false;                           // this "on"'s one switch is spent
        Switch switch_ = Switch::none;
        std::string problem_;                               // the last report that failed, in words
        int lightsSent_ = 0;                                // the mask last sent (or tried) while read
    };
}
