// shell/input/g29/G29.cpp: G29.h. Ported from TestingPlayground's Source/plugin/WheelMap.cpp (47bac40; SEED.tsv), the
// MIDI taken out.
#include "input/g29/G29.h"

#include <algorithm>
#include <iterator>

namespace tee::input::g29
{
    namespace
    {
        constexpr std::uint32_t padBit(Pad p) noexcept
        {
            switch (p)
            {
                case Pad::up: return 1u;
                case Pad::right: return 2u;
                case Pad::down: return 4u;
                case Pad::left: return 8u;
                case Pad::none: return 0u;
            }
            return 0u;
        }
    }

    Kind kindOf(int vendor, int product, int version) noexcept
    {
        if (vendor != kVendor)
            return Kind::none;
        if (product == kG29)
            return Kind::g29;
        if (product == kG29Ps4)
            return Kind::ps4;
        if (product == kCompat)
        {
            // lg4ff_main_checklist's first two rows, the G29's; any other bcdDevice is another wheel, left alone
            const int bcd = version & 0xFFFF;
            if ((bcd & 0xFFF8) == 0x1350 || (bcd & 0xFF00) == 0x8900)
                return Kind::compat;
        }
        return Kind::none;
    }

    bool isG29(int vendor, int product) noexcept
    {
        return vendor == kVendor && (product == kG29 || product == kCompat || product == kG29Ps4);
    }

    Report rangeReport(int degrees) noexcept
    {
        const int d = std::clamp(degrees, 40, 900);
        return { 0xf8, 0x81, static_cast<std::uint8_t>(d & 0xFF), static_cast<std::uint8_t>((d >> 8) & 0xFF), 0x00, 0x00, 0x00 };
    }

    std::array<Report, 2> springReports(std::uint16_t magnitude) noexcept
    {
        std::uint32_t a = 0, b = 0;
        const std::uint32_t m = magnitude;
        if (m <= 0xAAAAu)
        {
            a = 0x0Cu * m;
            b = 0x80u * m;
        }
        else
        {
            a = 0x0Cu * 0xAAAAu + 0x06u * (m - 0xAAAAu);
            b = 0x80u * 0xAAAAu + 0xFFu * (m - 0xAAAAu);
        }
        a >>= 1;                                             // "Adjust for non-MOMO wheels"
        const auto k = static_cast<std::uint8_t>(a / 0xAAAAu), clip = static_cast<std::uint8_t>(b / 0xAAAAu);
        return { { { 0xfe, 0x0d, k, k, clip, 0x00, 0x00 }, { 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } } };
    }

    Report lightsReport(int mask) noexcept
    {
        return { 0xf8, 0x12, static_cast<std::uint8_t>(mask & kAllLights), 0x00, 0x00, 0x00, kLightsLast };
    }

    float State::steering() const noexcept
    {
        const float s = static_cast<float>(v14[0] - kCentre) / static_cast<float>(kMost - kCentre);
        return std::clamp(s, -1.0f, 1.0f);
    }

    float State::pedal(Axis a) const noexcept
    {
        return static_cast<float>(v14[static_cast<std::size_t>(a)]) / static_cast<float>(kMost);
    }

    void normalise(std::int64_t& value, std::int64_t& min, std::int64_t& max, std::uint32_t reportSize) noexcept
    {
        if (max >= min || reportSize == 0 || reportSize > 32)
            return;
        max = (std::int64_t{ 1 } << reportSize) - 1;
        min = 0;
        value &= max;
    }

    int Map::axisValue(Axis a, bool restAtMax, std::int64_t value, std::int64_t min, std::int64_t max) noexcept
    {
        if (a == Axis::steering)
        {
            const std::int64_t span = max - min + 1;
            if (span <= 1)
                return kCentre;
            return static_cast<int>(std::clamp<std::int64_t>((value - min) * (kMost + 1) / span, 0, kMost));
        }
        const std::int64_t span = max - min;
        if (span <= 0)
            return 0;
        const std::int64_t pressed = std::clamp<std::int64_t>(restAtMax ? max - value : value - min, 0, span);
        const std::int64_t dead = (span + kDeadband / 2) / kDeadband;   // 1 % of the travel (8 bits: 3 of 255)
        if (pressed <= dead || span <= dead)
            return 0;
        const std::int64_t travel = span - dead;
        return static_cast<int>(((pressed - dead) * kMost + travel / 2) / travel);
    }

    std::uint32_t Map::padBits(std::int64_t value, std::int64_t min, std::int64_t max) noexcept
    {
        constexpr std::uint32_t up = 1u, right = 2u, down = 4u, left = 8u;
        constexpr std::uint32_t eight[] = { up, up | right, right, right | down, down, down | left, left, left | up };
        constexpr std::uint32_t four[] = { up, right, down, left };
        const std::int64_t n = max - min + 1, i = value - min;
        if (i < 0 || i >= n)
            return 0;
        if (n == 8)
            return eight[static_cast<std::size_t>(i)];
        if (n == 4)
            return four[static_cast<std::size_t>(i)];
        return 0;
    }

    bool Map::input(std::uint32_t page, std::uint32_t usage, std::int64_t value, std::int64_t min, std::int64_t max) noexcept
    {
        if (page == kDesktop)
        {
            for (const AxisRow& row : kAxisRows)
                if (row.usage == usage)
                {
                    now_.v14[static_cast<std::size_t>(row.axis)] = axisValue(row.axis, row.restAtMax, value, min, max);
                    return true;
                }
            if (usage == kHat)
            {
                pad_ = padBits(value, min, max);
                derive();
                return true;
            }
            return false;
        }
        if (page != kButtonPage || usage < 1 || usage > static_cast<std::uint32_t>(kButtonsMax))
            return false;
        const std::uint64_t bit = std::uint64_t{ 1 } << (usage - 1);
        held_ = value != 0 ? (held_ | bit) : (held_ & ~bit);
        derive();
        const int button = static_cast<int>(usage);
        return std::any_of(std::begin(kGears), std::end(kGears), [button](const GearRow& g) { return g.button == button; })
            || std::any_of(std::begin(kBits), std::end(kBits), [button](const BitRow& b) { return b.button == button; });
    }

    void Map::derive() noexcept
    {
        const auto down = [this](int button) {
            return button >= 1 && button <= kButtonsMax && ((held_ >> (button - 1)) & 1u) != 0;
        };
        now_.gear = 0;
        for (const GearRow& g : kGears)
            if (down(g.button))
            {
                now_.gear = g.gear;
                break;
            }
        std::uint32_t mask = 0;
        for (std::size_t i = 0; i < std::size(kBits); ++i)
        {
            const BitRow& row = kBits[i];
            if (row.button > 0 ? down(row.button) : (pad_ & padBit(row.pad)) != 0)
                mask |= 1u << i;
        }
        const std::uint32_t rose = mask & ~now_.buttons;
        for (int b = 0; b < 32; ++b)
            if (((rose >> b) & 1u) != 0 && presses_[static_cast<std::size_t>(b)] < 255)
                ++presses_[static_cast<std::size_t>(b)];
        now_.buttons = mask;
    }

    Presses Map::takePresses() noexcept
    {
        const Presses p = presses_;
        presses_.fill(0);
        return p;
    }

    void Map::rest() noexcept
    {
        now_ = State{};
        presses_.fill(0);
        held_ = 0;
        pad_ = 0;
    }

    std::string bitNames()
    {
        std::string s;
        for (std::size_t i = 0; i < std::size(kBits); ++i)
            s += (i > 0 ? ", " : "") + std::to_string(i) + " " + kBits[i].name;
        return s;
    }

    static_assert(std::size(kBits) <= 32, "the buttons' mask holds at most 32 bits");
}
