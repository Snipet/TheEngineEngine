// shell/input/g29/RacingWheelMac.cpp: RacingWheel.h on macOS, over IOKit's HID manager. Ported from TestingPlayground's
// Source/plugin/standalone/RacingWheel.cpp (47bac40; SEED.tsv), the MIDI and its flush timer taken out (the state is
// read a frame at a time). Compiled by CI's macOS build; run by no one but the owner.
#include "input/g29/RacingWheel.h"

#include "input/g29/G29Driver.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDLib.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace tee::input::g29
{
    namespace
    {
        constexpr CFTimeInterval kNever = 1.0e10;
        static_assert(Driver::kNotPermitted == static_cast<int>(kIOReturnNotPermitted), "the driver's words for it");

        int intProperty(IOHIDDeviceRef device, CFStringRef key)
        {
            const CFTypeRef v = IOHIDDeviceGetProperty(device, key);
            int out = 0;
            if (v != nullptr && CFGetTypeID(v) == CFNumberGetTypeID())
                CFNumberGetValue(static_cast<CFNumberRef>(v), kCFNumberIntType, &out);
            return out;
        }

        Kind kindOfDevice(IOHIDDeviceRef device)
        {
            return kindOf(intProperty(device, CFSTR(kIOHIDVendorIDKey)), intProperty(device, CFSTR(kIOHIDProductIDKey)),
                          intProperty(device, CFSTR(kIOHIDVersionNumberKey)));
        }

        // A matching dictionary: this vendor and product.
        CFDictionaryRef matching(int vendor, int product)
        {
            CFMutableDictionaryRef d = CFDictionaryCreateMutable(kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks,
                                                                 &kCFTypeDictionaryValueCallBacks);
            const CFNumberRef v = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendor);
            const CFNumberRef p = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &product);
            CFDictionarySetValue(d, CFSTR(kIOHIDVendorIDKey), v);
            CFDictionarySetValue(d, CFSTR(kIOHIDProductIDKey), p);
            CFRelease(v);
            CFRelease(p);
            return d;
        }
    }

    // The driver's transport (G29Driver.h decides): the HID manager, its devices, its two timers.
    struct RacingWheel::Impl final : Transport
    {
        Hooks hooks;
        IOHIDManagerRef manager = nullptr;
        std::vector<IOHIDDeviceRef> held;                    // every G29 the driver was told of (retained)
        IOHIDDeviceRef listening = nullptr;                  // the one whose values go to the driver
        CFRunLoopTimerRef timers[2] = { nullptr, nullptr };  // Timer's
        Driver driver;

        Impl(Hooks h, bool o) : hooks(std::move(h)), driver(*this, o)
        {
            for (const Timer t : { Timer::switched, Timer::lights })
                timers[static_cast<int>(t)] = timer(t);
            manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
            const void* each[] = { matching(kVendor, kG29), matching(kVendor, kCompat), matching(kVendor, kG29Ps4) };
            const CFArrayRef all = CFArrayCreate(kCFAllocatorDefault, each, 3, &kCFTypeArrayCallBacks);
            IOHIDManagerSetDeviceMatchingMultiple(manager, all);
            CFRelease(all);
            for (const void* d : each)
                CFRelease(d);
            IOHIDManagerRegisterDeviceMatchingCallback(manager, &Impl::onAdded, this);
            IOHIDManagerRegisterDeviceRemovalCallback(manager, &Impl::onRemoved, this);
            IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), kCFRunLoopCommonModes);
            driver.opened(IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone));   // shared: a game may open it too
        }

        ~Impl() override
        {
            driver.quit();                                   // the application quits: the wheel turns freely again
            IOHIDManagerRegisterDeviceMatchingCallback(manager, nullptr, nullptr);
            IOHIDManagerRegisterDeviceRemovalCallback(manager, nullptr, nullptr);
            IOHIDManagerRegisterInputValueCallback(manager, nullptr, nullptr);
            IOHIDManagerUnscheduleFromRunLoop(manager, CFRunLoopGetMain(), kCFRunLoopCommonModes);
            IOHIDManagerClose(manager, kIOHIDOptionsTypeNone);
            CFRelease(manager);
            for (IOHIDDeviceRef d : held)
                CFRelease(d);
            for (CFRunLoopTimerRef t : timers)
            {
                CFRunLoopTimerInvalidate(t);
                CFRelease(t);
            }
        }

        // A timer on the main run loop, in every mode, that fires only when armed, once each time.
        CFRunLoopTimerRef timer(Timer which)
        {
            CFRunLoopTimerContext context{ 0, this, nullptr, nullptr, nullptr };
            const auto fire = which == Timer::switched ? &Impl::onSwitched : &Impl::onLights;
            CFRunLoopTimerRef t = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + kNever, kNever, 0, 0,
                                                       fire, &context);
            CFRunLoopAddTimer(CFRunLoopGetMain(), t, kCFRunLoopCommonModes);
            return t;
        }

        static IOHIDDeviceRef ref(Device d) { return static_cast<IOHIDDeviceRef>(const_cast<void*>(d)); }

        // ---- Transport --------------------------------------------------------------------------------------------
        int report(Device d, const Report& r) override
        {
            return IOHIDDeviceSetReport(ref(d), kIOHIDReportTypeOutput, 0, r.data(), static_cast<CFIndex>(r.size()));
        }

        // Every input element's value as IOKit holds it, with its timestamp (0: no report carried it yet).
        std::vector<Value> values(Device d) override
        {
            std::vector<Value> out;
            const CFArrayRef elements = IOHIDDeviceCopyMatchingElements(ref(d), nullptr, kIOHIDOptionsTypeNone);
            if (elements == nullptr)
                return out;
            for (CFIndex i = 0, n = CFArrayGetCount(elements); i < n; ++i)
            {
                const auto e = static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements, i)));
                const IOHIDElementType type = IOHIDElementGetType(e);
                if (type != kIOHIDElementTypeInput_Misc && type != kIOHIDElementTypeInput_Button
                    && type != kIOHIDElementTypeInput_Axis)
                    continue;
                IOHIDValueRef value = nullptr;
                if (IOHIDDeviceGetValue(ref(d), e, &value) == kIOReturnSuccess && value != nullptr)
                    out.push_back(valueOf(e, value));
            }
            CFRelease(elements);
            return out;
        }

        void listen(Device d) override
        {
            listening = ref(d);
            IOHIDManagerRegisterInputValueCallback(manager, d != nullptr ? &Impl::onValue : nullptr,
                                                   d != nullptr ? this : nullptr);
        }

        void arm(Timer t, double seconds) override
        {
            CFRunLoopTimerSetNextFireDate(timers[static_cast<int>(t)], CFAbsoluteTimeGetCurrent() + seconds);
        }

        void disarm(Timer t) override
        {
            CFRunLoopTimerSetNextFireDate(timers[static_cast<int>(t)], CFAbsoluteTimeGetCurrent() + kNever);
        }

        void changed() override
        {
            if (hooks.changed)
                hooks.changed();
        }

        int lights() override { return hooks.lights ? hooks.lights() : -1; }

        // ---- IOKit's callbacks ---------------------------------------------------------------------------------------
        static Value valueOf(IOHIDElementRef e, IOHIDValueRef value)
        {
            Value v;
            v.page = IOHIDElementGetUsagePage(e);
            v.usage = IOHIDElementGetUsage(e);
            v.value = IOHIDValueGetIntegerValue(value);
            v.min = IOHIDElementGetLogicalMin(e);
            v.max = IOHIDElementGetLogicalMax(e);
            v.reportSize = IOHIDElementGetReportSize(e);
            v.timestamp = IOHIDValueGetTimeStamp(value);
            return v;
        }

        static void onAdded(void* context, IOReturn, void*, IOHIDDeviceRef device)
        {
            auto* self = static_cast<Impl*>(context);
            const Kind kind = kindOfDevice(device);
            if (kind == Kind::none || std::find(self->held.begin(), self->held.end(), device) != self->held.end())
                return;                                      // another Logitech wheel in compatibility mode
            CFRetain(device);
            self->held.push_back(device);
            self->driver.added(device, kind);
        }

        static void onRemoved(void* context, IOReturn, void*, IOHIDDeviceRef device)
        {
            auto* self = static_cast<Impl*>(context);
            const auto it = std::find(self->held.begin(), self->held.end(), device);
            if (it == self->held.end())
                return;
            self->driver.removed(device);                    // before it is let go
            if (self->listening == device)
                self->listen(nullptr);
            CFRelease(*it);
            self->held.erase(it);
        }

        static void onValue(void* context, IOReturn, void*, IOHIDValueRef value)
        {
            auto* self = static_cast<Impl*>(context);
            const IOHIDElementRef e = IOHIDValueGetElement(value);
            const IOHIDDeviceRef device = IOHIDElementGetDevice(e);
            if (self->listening != nullptr && device == self->listening)
                self->driver.value(device, valueOf(e, value));
        }

        static void onSwitched(CFRunLoopTimerRef, void* info) { static_cast<Impl*>(info)->driver.fire(Timer::switched); }
        static void onLights(CFRunLoopTimerRef, void* info) { static_cast<Impl*>(info)->driver.fire(Timer::lights); }
    };

    RacingWheel::RacingWheel(Hooks hooks, bool on) : impl_(std::make_unique<Impl>(std::move(hooks), on)) {}
    RacingWheel::~RacingWheel() = default;

    bool RacingWheel::supported() noexcept { return true; }
    bool RacingWheel::present() const noexcept { return impl_->driver.present(); }
    bool RacingWheel::on() const noexcept { return impl_->driver.on(); }
    void RacingWheel::setOn(bool on) { impl_->driver.setOn(on); }
    bool RacingWheel::reading() const noexcept { return impl_->driver.reading(); }
    State RacingWheel::state() const { return impl_->driver.state(); }
    Presses RacingWheel::takePresses() { return impl_->driver.takePresses(); }
    std::string RacingWheel::status() const { return impl_->driver.status(); }

    // SDL's Cocoa pump (SDL_PumpEvents: Cocoa_PumpEventsUntilDate, [NSApp nextEventMatchingMask:... untilDate:
    // distantPast inMode:NSDefaultRunLoopMode]) runs the main run loop in its default mode, which services sources and
    // timers scheduled in the common modes; this runs it once more, returning at once, so the wheel's sources and timers
    // run each frame whatever the window's pump does (a frame drawn while SDL waits in a modal loop included).
    void RacingWheel::pump() { CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, true); }
    bool RacingWheel::clutchAxis() const noexcept { return true; }
}
