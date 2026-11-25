// tests/shell/G29EvdevTest.cpp: the G29 on Linux (input/g29/G29Evdev.h's Hub) over a made-up sysfs tree and nodes of the
// test's own (a socket a node: the test writes the kernel's events into it and reads what the hub writes). Ported from
// TestingPlayground's Tools/probes/plugin/linux.cpp, its WHEEL cases (47bac40; g29/SEED.tsv), the MIDI taken out (the
// driver's state read instead). Nothing of the machine's is read or written: the tree is under out/shell/.
//   the G29 found among other devices (a keyboard of Logitech's, another vendor's; a G27 in compatibility mode none, a
//   G29 in it found); hid-input's codes made HID usages; on, a range file already 900 left alone (read-only), the spring
//   as FF_AUTOCENTER 0x6000 on the node; its events into the state; the PS button the start key; the shift lights in
//   RPM1 to RPM5's brightness; SYN_DROPPED's events dropped until SYN_REPORT, then the state read again; off: the
//   spring 0, the lights 0; unplugged: at rest, no longer present; plugged in again: read again, a range of 540 written
//   900; lights it may not write said; a range of 540 it may not write said and no spring; a node it may not open
//   listed and said, read once it may; a G29 in compatibility mode switched through alternate_modes; the hub gone: the
//   spring off.
#if defined(__linux__)
#include "input/g29/G29Evdev.h"

#include <catch2/catch_test_macros.hpp>

#include <linux/input.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace wh = tee::input::g29;
namespace ev = tee::input::g29::evdev;

namespace
{
    void writeText(const fs::path& p, const std::string& text)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p, std::ios::binary | std::ios::trunc) << text;
    }

    std::string readText(const fs::path& p)
    {
        std::ifstream in(p, std::ios::binary);
        std::stringstream s;
        s << in.rdbuf();
        return s.str();
    }

    // The made-up kernel: nodes are sockets, the axes' and keys' state is the test's.
    struct Kernel final : ev::System
    {
        std::map<std::string, int> peers;                    // a node's path -> the test's end of its socket
        std::set<std::string> denied;                        // nodes open() refuses (EACCES)
        std::set<int> watched;
        std::map<std::uint16_t, std::array<std::int32_t, 3>> absState{ { ABS_X, { 32768, 0, 65535 } },
                                                                       { ABS_Y, { 255, 0, 255 } },
                                                                       { ABS_Z, { 255, 0, 255 } },
                                                                       { ABS_RZ, { 255, 0, 255 } },
                                                                       { ABS_HAT0X, { 0, -1, 1 } },
                                                                       { ABS_HAT0Y, { 0, -1, 1 } } };
        std::set<std::uint16_t> keysDown;
        bool armed[2] = { false, false };
        int asked = -1;

        ~Kernel() override
        {
            for (const auto& [node, fd] : peers)
                ::close(fd);
        }

        int open(const std::string& node) override
        {
            if (denied.count(node) != 0)
            {
                errno = EACCES;
                return -1;
            }
            if (!fs::exists(node))
            {
                errno = ENOENT;
                return -1;
            }
            int sv[2];
            if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv) != 0)
                return -1;
            if (const auto it = peers.find(node); it != peers.end())
                ::close(it->second);
            peers[node] = sv[1];
            return sv[0];
        }
        bool abs(int, std::uint16_t code, std::int32_t& value, std::int32_t& min, std::int32_t& max) override
        {
            const auto it = absState.find(code);
            if (it == absState.end())
                return false;
            value = it->second[0];
            min = it->second[1];
            max = it->second[2];
            return true;
        }
        bool keys(int, std::vector<std::uint8_t>& bits) override
        {
            bits.assign(KEY_MAX / 8 + 1, 0);
            for (const std::uint16_t k : keysDown)
                bits[k / 8] = static_cast<std::uint8_t>(bits[k / 8] | (1u << (k % 8)));
            return true;
        }
        void watch(int fd) override { watched.insert(fd); }
        void unwatch(int fd) override { watched.erase(fd); }
        void arm(wh::Timer t, double) override { armed[static_cast<int>(t)] = true; }
        void disarm(wh::Timer t) override { armed[static_cast<int>(t)] = false; }
        void changed() override {}
        int lights() override { return asked; }

        void events(const std::string& node, const std::vector<input_event>& es)
        {
            const int fd = peers.at(node);
            for (const input_event& e : es)
                (void)::write(fd, &e, sizeof e);
        }
        // What the hub wrote into a node (its EV_FF events), as "ff 97:24576 ...".
        std::string written(const std::string& node)
        {
            std::string s;
            input_event e{};
            while (::read(peers.at(node), &e, sizeof e) == static_cast<ssize_t>(sizeof e))
                s += (s.empty() ? "" : " ") + std::string(e.type == EV_FF ? "ff " : "? ") + std::to_string(e.code) + ":"
                   + std::to_string(e.value);
            return s;
        }
        void pump(ev::Hub& hub)
        {
            const std::set<int> now = watched;
            for (const int fd : now)
                hub.readable(fd);
        }
        void fire(ev::Hub& hub, wh::Timer t)
        {
            armed[static_cast<int>(t)] = false;
            hub.driver().fire(t);
        }
    };

    input_event inputEvent(std::uint16_t type, std::uint16_t code, std::int32_t value, long micros = 1000)
    {
        input_event e{};
        e.input_event_sec = micros / 1000000;
        e.input_event_usec = micros % 1000000;
        e.type = type;
        e.code = code;
        e.value = value;
        return e;
    }

    // A made-up sysfs input device: class/input/<event>/device -> devices/<input> (a link, as the kernel's), its ids, and
    // its HID device's folder with hid-lg4ff's files.
    fs::path makeDevice(const fs::path& sys, const fs::path& dev, const std::string& event, const std::string& input,
                        int vendor, int product, int version, bool lg4ff)
    {
        const fs::path in = sys / "devices" / input;
        char hex[8];
        std::snprintf(hex, sizeof hex, "%04x", vendor);
        writeText(in / "id/vendor", std::string(hex) + "\n");
        std::snprintf(hex, sizeof hex, "%04x", product);
        writeText(in / "id/product", std::string(hex) + "\n");
        std::snprintf(hex, sizeof hex, "%04x", version);
        writeText(in / "id/version", std::string(hex) + "\n");
        const fs::path hid = in / "device";
        fs::create_directories(hid);
        if (lg4ff)
        {
            writeText(hid / "range", "900\n");
            writeText(hid / "alternate_modes", "");
            for (int i = 1; i <= 5; ++i)
                writeText(hid / "leds" / ("0003:046D:C24F.0001::RPM" + std::to_string(i)) / "brightness", "0\n");
        }
        fs::create_directories(sys / "class/input" / event);
        std::error_code ec;
        fs::remove(sys / "class/input" / event / "device", ec);
        fs::create_directory_symlink(fs::path("../../../devices") / input, sys / "class/input" / event / "device");
        writeText(dev / "input" / event, "");
        return hid;
    }

    void unplug(const fs::path& sys, const fs::path& dev, const std::string& event)
    {
        std::error_code ec;
        fs::remove_all(sys / "class/input" / event, ec);
        fs::remove(dev / "input" / event, ec);
    }

    std::string leds(const fs::path& hid)
    {
        std::string s;
        for (int i = 1; i <= 5; ++i)
        {
            std::string b = readText(hid / "leds" / ("0003:046D:C24F.0001::RPM" + std::to_string(i)) / "brightness");
            while (!b.empty() && b.back() == '\n')
                b.pop_back();
            s += b;
        }
        return s;
    }
}

TEST_CASE("g29 on Linux: the Hub over a made-up kernel: found, read, its range, spring and lights, dropped, unplugged, "
          "denied, switched",
          "[shell][input][g29][linux]")
{
    const fs::path out = fs::path(TEE_SOURCE_DIR) / "out" / "shell" / "g29-linux";
    std::error_code ec;
    fs::remove_all(out, ec);
    const fs::path sys = out / "sys", dev = out / "dev";
    fs::create_directories(sys / "class/input");
    fs::create_directories(dev / "input");
    makeDevice(sys, dev, "event3", "input20", 0x046d, 0xc31c, 0x6400, false);   // a Logitech keyboard
    makeDevice(sys, dev, "event4", "input21", 0x1234, 0xc24f, 0x0100, false);   // another vendor's
    makeDevice(sys, dev, "event6", "input23", 0x046d, 0xc294, 0x1230, false);   // a G27 in compatibility mode
    const fs::path hid = makeDevice(sys, dev, "event5", "input22", 0x046d, 0xc24f, 0x0100, true);
    const std::string node = (dev / "input/event5").string();
    const ev::Paths paths{ sys.string(), dev.string() };

    // SCAN and the mapping
    {
        const auto found = ev::scan(paths);
        std::string s;
        for (const ev::Found& f : found)
            s += f.event + "/" + f.input + " ";
        CHECK(s == "event5/input22 ");
        REQUIRE_FALSE(found.empty());
        CHECK(found[0].node == node);
        CHECK(found[0].kind == wh::Kind::g29);
        makeDevice(sys, dev, "event7", "input24", 0x046d, 0xc294, 0x1350, true);
        const auto two = ev::scan(paths);
        CHECK((two.size() == 2 && two[1].kind == wh::Kind::compat));
        unplug(sys, dev, "event7");
        CHECK(ev::buttonOf(BTN_TRIGGER) == 1);
        CHECK(ev::buttonOf(BTN_DEAD) == 16);
        CHECK(ev::buttonOf(BTN_TRIGGER_HAPPY1) == 17);
        CHECK(ev::buttonOf(BTN_TRIGGER_HAPPY9) == 25);
        CHECK(ev::buttonOf(BTN_SOUTH) == 1);
        CHECK(ev::buttonOf(KEY_A) == 0);
        CHECK(ev::usageOf(ABS_X) == 0x30);
        CHECK(ev::usageOf(ABS_RZ) == 0x35);
        CHECK(ev::usageOf(ABS_HAT0X) == 0);
        std::string hats;
        for (const auto& [x, y] : std::vector<std::pair<int, int>>{ { 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },
                                                                     { -1, 0 }, { -1, -1 }, { 0, 0 } })
            hats += std::to_string(ev::hatOf(x, y));
        CHECK(hats == "012345678");
    }

    Kernel k;
    fs::permissions(hid / "range", fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read);
    {
        ev::Hub hub(k, paths, true);
        hub.rescan();
        CHECK(hub.driver().present());
        CHECK(hub.driver().reading());
        CHECK(k.watched.size() == 1);
        CHECK(hub.driver().status().empty());
        CHECK(readText(hid / "range") == "900\n");               // as the kernel set it: left alone
        CHECK(k.written(node) == "ff 97:24576");
        CHECK(hub.driver().state() == wh::State{});              // nothing reported yet: at rest

        // events: the steering full right, the throttle down, cross, the d-pad right; then the PS button
        k.events(node, { inputEvent(EV_ABS, ABS_X, 65535), inputEvent(EV_ABS, ABS_Z, 0), inputEvent(EV_KEY, BTN_TRIGGER, 1),
                         inputEvent(EV_ABS, ABS_HAT0X, 1), inputEvent(EV_SYN, SYN_REPORT, 0) });
        k.pump(hub);
        wh::Map ref;
        ref.input(wh::kDesktop, 0x30, 65535, 0, 65535);
        ref.input(wh::kDesktop, 0x32, 0, 0, 255);
        ref.input(wh::kButtonPage, 1, 1, 0, 1);
        ref.input(wh::kDesktop, wh::kHat, 2, 0, 7);
        CHECK(hub.driver().state() == ref.state());              // the same as a map of the same values
        k.events(node, { inputEvent(EV_KEY, BTN_TRIGGER_HAPPY9, 1, 2000), inputEvent(EV_SYN, SYN_REPORT, 0, 2000) });
        k.pump(hub);
        CHECK(hub.driver().state().button(wh::kBitPs));          // the start key

        // the shift lights
        k.asked = 0x05;
        k.fire(hub, wh::Timer::lights);
        CHECK(leds(hid) == "10100");
        k.asked = 0x1F;
        k.fire(hub, wh::Timer::lights);
        CHECK(leds(hid) == "11111");

        // SYN_DROPPED: dropped until SYN_REPORT, then the state as the kernel holds it (the throttle now half way)
        k.absState[ABS_X] = { 65535, 0, 65535 };
        k.absState[ABS_HAT0X] = { 1, -1, 1 };
        k.keysDown = { BTN_TRIGGER, BTN_TRIGGER_HAPPY9 };
        k.absState[ABS_Z] = { 128, 0, 255 };
        k.events(node, { inputEvent(EV_SYN, SYN_DROPPED, 0, 3000), inputEvent(EV_ABS, ABS_Z, 5, 3000),
                         inputEvent(EV_SYN, SYN_REPORT, 0, 3000) });
        k.pump(hub);
        ref.input(wh::kButtonPage, 25, 1, 0, 1);
        ref.input(wh::kDesktop, 0x32, 128, 0, 255);
        CHECK(hub.driver().state() == ref.state());

        // off: the spring 0, the lights off, at rest
        hub.driver().setOn(false);
        CHECK(k.written(node) == "ff 97:0");
        CHECK(leds(hid) == "00000");
        CHECK(hub.driver().state() == wh::State{});
        hub.driver().setOn(true);
        CHECK(k.written(node) == "ff 97:24576");

        // unplugged: the node's end, then the kernel's list without it
        ::close(k.peers.at(node));
        k.peers.erase(node);
        k.pump(hub);
        CHECK_FALSE(hub.driver().present());
        CHECK(k.watched.empty());
        CHECK(hub.driver().state() == wh::State{});
        unplug(sys, dev, "event5");
        hub.rescan();
        CHECK_FALSE(hub.driver().present());

        // plugged in again (another input device), its range 540: written 900
        const fs::path hid2 = makeDevice(sys, dev, "event5", "input25", 0x046d, 0xc24f, 0x0100, true);
        writeText(hid2 / "range", "540\n");
        hub.rescan();
        CHECK(hub.driver().present());
        CHECK(readText(hid2 / "range") == "900");
        CHECK(k.written(node) == "ff 97:24576");

        // lights it may not write: said
        for (int i = 1; i <= 5; ++i)
            fs::permissions(hid2 / "leds" / ("0003:046D:C24F.0001::RPM" + std::to_string(i)) / "brightness", fs::perms::owner_read);
        k.asked = 0x03;
        k.fire(hub, wh::Timer::lights);
        if (::geteuid() != 0)                                    // (root writes anything: the check needs a user)
            CHECK(hub.driver().status() == "its shift lights were not set (it is root's: see docs/controls.md for a udev rule)");
        k.asked = -1;
    }
    CHECK(k.written(node) == "ff 97:0");                         // the hub gone: the spring off

    // a range of 540 it may not write: said, and no spring
    if (::geteuid() != 0)
    {
        writeText(sys / "devices/input25/device/range", "540\n");
        fs::permissions(sys / "devices/input25/device/range", fs::perms::owner_read);
        ev::Hub hub(k, paths, true);
        hub.rescan();
        CHECK(hub.driver().status() == "its range and spring were not set (it is root's: see docs/controls.md for a udev rule)");
        CHECK(k.written(node).empty());
        fs::permissions(sys / "devices/input25/device/range", fs::perms::owner_read | fs::perms::owner_write);
    }
    k.written(node);

    // a node it may not open: listed and said; read once it may
    {
        k.denied.insert(node);
        ev::Hub hub(k, paths, true);
        hub.rescan();
        CHECK(hub.driver().present());
        CHECK(hub.driver().status()
              == "the system keeps it from this app: its device needs your read and write access (see docs/controls.md)");
        k.denied.clear();
        hub.rescan();
        CHECK(k.written(node) == "ff 97:24576");
        CHECK(hub.driver().status().empty());
    }
    k.written(node);
    unplug(sys, dev, "event5");

    // a G29 in compatibility mode (the kernel told not to switch it): "native" to alternate_modes
    {
        const fs::path hid3 = makeDevice(sys, dev, "event8", "input26", 0x046d, 0xc294, 0x1350, true);
        ev::Hub hub(k, paths, true);
        hub.rescan();
        CHECK(readText(hid3 / "alternate_modes") == "native");
        CHECK(hub.driver().status() == "switching to its full mode");
        CHECK(k.armed[static_cast<int>(wh::Timer::switched)]);
    }
    fs::remove_all(out, ec);
}
#endif
