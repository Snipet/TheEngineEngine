// shell/input/g29/G29.h: a Logitech G29 racing wheel (the wheel, its pedals and the Driving Force Shifter on one USB
// cable), in plain C++ (no IOKit, no SDL): which device a USB HID one is, the commands it takes, the table of its
// elements, and Map, its element values made the wheel's state (pedals, steering, gear, buttons) that
// g29/G29Input.h hands to the Driver. Ported from TestingPlayground's Source/plugin/WheelMap.{h,cpp} (47bac40; SEED.tsv;
// docs/reference/testingplayground-wheel/), where it drove the owner's G29 on a Mac: the MIDI is taken out (the state
// is read a frame at a time), the table, the commands and the rules are as they were.
//
// The device (vendor 0x046D), as Linux's hid-lg4ff tells them apart (drivers/hid/hid-lg4ff.c):
//   0xC24F  a G29 in its own mode ("native"): read
//   0xC294  a Logitech wheel's compatibility mode (Driving Force EX), as a G29 with its switch at PS3 starts without
//           Logitech's software: 10-bit steering, no clutch, no shifter. A G29 is told apart by its bcdDevice
//           (lg4ff_g29_ident_info and _info2: & 0xFFF8 == 0x1350, or & 0xFF00 == 0x8900) and switched to its own mode
//           with hid-lg4ff's two reports (kToNative), after which it leaves and comes back as 0xC24F
//   0xC260  a G29 with its switch at PS4: not read (the status line says to set the switch to PS3)
//
// The table (kAxisRows, kGears, kBits): TestingPlayground's, which its header calls a guess checked on no device; the
// owner says the G29 worked with it, lights included. A row changed here is the whole fix; tee-wheel-dump (to come)
// shows what a connected wheel has.
//
// Never run with a real wheel in this repository: tests/shell/G29Test.cpp drives it with made-up values.
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace tee::input::g29
{
    inline constexpr int kVendor = 0x046D;                   // Logitech
    inline constexpr int kG29 = 0xC24F;                      // hid-ids.h's USB_DEVICE_ID_LOGITECH_G29_WHEEL
    inline constexpr int kCompat = 0xC294;                   // USB_DEVICE_ID_LOGITECH_WHEEL (Driving Force EX)
    inline constexpr int kG29Ps4 = 0xC260;                   // a G29 at PS4

    // What a USB device is to us.
    enum class Kind : std::uint8_t
    {
        none,       // not a G29 (another Logitech wheel in compatibility mode among them: left alone)
        g29,        // a G29 in its own mode: read it
        compat,     // a G29 in compatibility mode: switch it (kToNative)
        ps4,        // a G29 at PS4
    };
    // By its vendor and product ids and its version (bcdDevice), as hid-lg4ff's lg4ff_identify_multimode_wheel.
    Kind kindOf(int vendor, int product, int version) noexcept;
    // Whether SDL's own joystick or gamepad path must leave this device alone (any of the three ids: the G29 is ours).
    bool isG29(int vendor, int product) noexcept;

    // An output report of the wheel's (report id 0, 7 bytes), as hid-lg4ff sends them (hid_hw_request, SET_REPORT).
    using Report = std::array<std::uint8_t, 7>;

    // hid-lg4ff's lg4ff_mode_switch_ext09_g29: revert the mode upon a USB reset, then switch to G29 mode with a detach.
    inline constexpr std::array<Report, 2> kToNative{ { { 0xf8, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                                        { 0xf8, 0x09, 0x05, 0x01, 0x01, 0x00, 0x00 } } };
    // hid-lg4ff's lg4ff_set_range_g25 (the G25, G27, DFGT and G29's): the rotation, 40 to 900 degrees.
    Report rangeReport(int degrees) noexcept;
    // hid-lg4ff's lg4ff_set_autocenter_default, magnitude 1 to 0xFFFF: the spring's strength and limit, then "activate
    // auto-centre". Its 0, deactivated, is kSpringOff.
    std::array<Report, 2> springReports(std::uint16_t magnitude) noexcept;
    inline constexpr Report kSpringOff{ { 0xf5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

    inline constexpr int kRange = 900;                       // degrees lock to lock, as the wheel's own default
    inline constexpr std::uint16_t kSpring = 0x6000;         // a moderate spring (a game's 37 %): fe 0d 03 03 48

    // ---- the shift lights ----------------------------------------------------------------------------------------
    // The five shift lights (the LEDs over the wheel's hub, a pair each, lit from the outside in as a tach rises), as
    // hid-lg4ff drives them (lg4ff_set_leds): a mask, bit 0 the first to light (the outermost pair), bit 4 the last.
    inline constexpr int kLights = 5;
    inline constexpr int kAllLights = (1 << kLights) - 1;    // 0x1F
    // The report: the extended command f8 12, the mask, then 00 00 00 01. TestingPlayground's note: the last byte was
    // written from memory (01, as the G27's protocol notes have it; the driver may send 00); the owner says the lights
    // worked with it. kLightsLast is the one place to change it.
    inline constexpr std::uint8_t kLightsLast = 0x01;
    Report lightsReport(int mask) noexcept;                  // the mask's low five bits
    // The first `lit` lights' mask (0..5: 0 none, 5 all).
    constexpr int litMask(int lit) noexcept { return lit <= 0 ? 0 : lit >= kLights ? kAllLights : (1 << lit) - 1; }

    // ---- the table: the G29's elements in its own mode, by HID usage -----------------------------------------------
    inline constexpr std::uint32_t kDesktop = 0x01;          // the Generic Desktop page
    inline constexpr std::uint32_t kButtonPage = 0x09;
    inline constexpr std::uint32_t kHat = 0x39;              // the hat switch (Generic Desktop): the d-pad

    enum class Axis : std::uint8_t { steering, throttle, brake, clutch };
    inline constexpr int kAxes = 4;

    struct AxisRow
    {
        std::uint32_t usage;     // on kDesktop
        Axis axis;
        bool restAtMax;          // its logical maximum at rest (Logitech's pedals read their maximum when released)
        const char* name;
    };
    inline constexpr AxisRow kAxisRows[] = {
        { 0x30, Axis::steering, false, "X: steering" },      // 16 bits, 0 to 65535
        { 0x32, Axis::throttle, true, "Z: throttle" },       // 8 bits
        { 0x35, Axis::brake, true, "Rz: brake" },
        { 0x31, Axis::clutch, true, "Y: clutch" },
    };

    // The Driving Force Shifter's positions, buttons (1-based) on kButtonPage; none of them down: neutral.
    struct GearRow
    {
        int button;
        int gear;                // -1 reverse
    };
    inline constexpr GearRow kGears[] = { { 13, 1 }, { 14, 2 }, { 15, 3 }, { 16, 4 }, { 17, 5 }, { 18, 6 }, { 19, -1 } };

    // The buttons' mask, a row a bit (bit i is kBits[i]): a button (1-based, on kButtonPage) or a direction of the
    // d-pad. The shifter's buttons are the gear, not bits.
    enum class Pad : std::uint8_t { none, up, right, down, left };
    struct BitRow
    {
        int button;              // 0: a direction of the d-pad
        Pad pad;
        const char* name;
    };
    inline constexpr BitRow kBits[] = {
        { 1, Pad::none, "cross" },           { 2, Pad::none, "square" },        { 3, Pad::none, "circle" },
        { 4, Pad::none, "triangle" },        { 5, Pad::none, "right paddle" },  { 6, Pad::none, "left paddle" },
        { 7, Pad::none, "R2" },              { 8, Pad::none, "L2" },            { 9, Pad::none, "share" },
        { 10, Pad::none, "options" },        { 11, Pad::none, "R3" },           { 12, Pad::none, "L3" },
        { 20, Pad::none, "+" },              { 21, Pad::none, "-" },            { 22, Pad::none, "dial right" },
        { 23, Pad::none, "dial left" },      { 24, Pad::none, "return" },       { 25, Pad::none, "PS" },
        { 0, Pad::up, "d-pad up" },          { 0, Pad::right, "d-pad right" },  { 0, Pad::down, "d-pad down" },
        { 0, Pad::left, "d-pad left" },
    };
    // Bits of the mask this application gives a meaning (docs/controls.md, "A wheel and pedals").
    inline constexpr int kBitRightPaddle = 4;
    inline constexpr int kBitLeftPaddle = 5;
    inline constexpr int kBitPs = 17;                        // the start key, held
    static_assert(kBits[kBitPs].button == 25 && kBits[kBitRightPaddle].button == 5 && kBits[kBitLeftPaddle].button == 6);

    inline constexpr int kButtonsMax = 64;                   // the buttons Map keeps (1 to 64)
    inline constexpr int kDeadband = 100;                    // a pedal's travel within 1 / kDeadband of rest: released
    inline constexpr int kCentre = 8192;                     // 14 bits: the steering's centre
    inline constexpr int kMost = 16383;

    // The wheel's state.
    struct State
    {
        std::array<int, kAxes> v14{ { kCentre, 0, 0, 0 } };   // steering, throttle, brake, clutch (14 bits)
        int gear = 0;                                        // -1 reverse, 0 neutral, 1..6
        std::uint32_t buttons = 0;                           // the mask (kBits)

        float steering() const noexcept;                     // -1 full left .. 1 full right
        float pedal(Axis a) const noexcept;                  // 0 released .. 1 down
        bool button(int bit) const noexcept { return bit >= 0 && bit < 32 && ((buttons >> bit) & 1u) != 0; }
        friend bool operator==(const State&, const State&) = default;
    };

    // Each bit's presses (rises of the mask) since they were last taken: the transports hand Map every value as it
    // comes (IOKit's input values, evdev's events), so a button pressed and let go between two frames, as a dial's
    // detent may be, is counted, not lost to the frame's sample. Saturates at 255.
    using Presses = std::array<std::uint8_t, 32>;

    // A G29's element values in, its state out. One thread, no locks.
    class Map
    {
    public:
        // The presses since the last take (each bit's count), and the count restarted.
        Presses takePresses() noexcept;
        // An element's new value (its usage page and usage, its logical minimum and maximum): false when the table has
        // no row for it (nothing changes).
        bool input(std::uint32_t page, std::uint32_t usage, std::int64_t value, std::int64_t min, std::int64_t max) noexcept;
        const State& state() const noexcept { return now_; }
        // At rest: centred, released, neutral, no button (the wheel gone or switched off: nothing stays held).
        void rest() noexcept;

        // An axis's 14-bit value of an element's (its logical range): the steering scaled so the range's middle is
        // 8192, a pedal so its rest is 0 and the far end 16383 (its travel past kDeadband spread over the whole range).
        static int axisValue(Axis a, bool restAtMax, std::int64_t value, std::int64_t min, std::int64_t max) noexcept;
        // The d-pad's direction bits (up 1, right 2, down 4, left 8) of a hat's value: 8 positions or 4; outside: none.
        static std::uint32_t padBits(std::int64_t value, std::int64_t min, std::int64_t max) noexcept;

    private:
        void derive() noexcept;

        State now_;
        Presses presses_{};
        std::uint64_t held_ = 0;                             // buttons 1 to 64 down, bit n - 1
        std::uint32_t pad_ = 0;
    };

    // An element's range as its descriptor meant it: a maximum below the minimum is an unsigned one read as signed
    // (Logical Maximum 65535 written in two bytes is -1), so the range becomes 0 to 2^reportSize - 1 and the value its
    // low reportSize bits. Anything else is left as it is.
    void normalise(std::int64_t& value, std::int64_t& min, std::int64_t& max, std::uint32_t reportSize) noexcept;

    inline constexpr const char* kName = "Logitech G29 Racing Wheel";
    // The mask's bits by name ("0 cross, 1 square, ..."), for the dump tool and the docs.
    std::string bitNames();
}
