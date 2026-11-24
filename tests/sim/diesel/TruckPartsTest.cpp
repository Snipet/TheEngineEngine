// tests/sim/diesel/TruckPartsTest.cpp: card D2's truck parts alone (rig.fm: sim/vehicle/roadranger.fm, truckair.fm):
// the eighteen-speed's lever, range and splitter, its unsynchronised dogs (floated in, or ground), and the air system's
// events.
#include "tee/Denormals.h"
#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr double kRate = 48000.0;

    std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    tee::Program& program()
    {
        // (never destroyed: 01-rules.md, found on macOS)
        static tee::Program* compiled = [] {
            tee::Program::Files files;
            const std::filesystem::path root = TEE_SOURCE_DIR;
            for (const char* f : {"vehicle/roadranger.fm", "vehicle/truckair.fm", "lib/noise.fm"})
                files[f] = readFile(root / "sim" / f);
            files["rig.fm"] = readFile(root / "tests" / "sim" / "diesel" / "rig.fm");
            std::string errors;
            auto p = tee::Program::fromFiles(files, "rig.fm", {}, errors);
            INFO(errors);
            REQUIRE(p != nullptr);
            REQUIRE(p->link(errors));
            return p.release();
        }();
        return *compiled;
    }

    struct Rig
    {
        std::unique_ptr<tee::Schema> schema;
        std::unique_ptr<tee::Instance> inst;
        void (*run_)(void*, float*, float*, int) = nullptr;
        std::vector<float> g, a;

        Rig()
        {
            std::string error;
            schema = tee::Schema::of(program().fminus(), "Rig", error);
            REQUIRE(schema != nullptr);
            inst = std::make_unique<tee::Instance>(*program().fminus().module("Rig"));
            auto* init = program().function<void(void*, float)>("Rig::init", error);
            REQUIRE(init != nullptr);
            run_ = program().function<void(void*, float*, float*, int)>("Rig::run", error);
            REQUIRE(run_ != nullptr);
            init(inst->data(), static_cast<float>(kRate));
        }

        void set(const std::string& path, double v)
        {
            std::string error;
            REQUIRE(inst->write(*schema, path, v, error));
        }

        double get(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            REQUIRE(inst->read(*schema, path, v, error));
            return v;
        }

        void run(double seconds)
        {
            const int n = static_cast<int>(seconds * kRate / 16.0 + 0.5) * 16;
            g.assign(static_cast<size_t>(n), 0.0f);
            a.assign(static_cast<size_t>(n), 0.0f);
            tee::ScopedFlushDenormals flush;
            run_(inst->data(), g.data(), a.data(), n);
        }

        static double rms(const std::vector<float>& x)
        {
            double s = 0.0;
            for (float v : x)
                s += static_cast<double>(v) * v;
            return std::sqrt(s / std::max<size_t>(x.size(), 1));
        }
    };

    const double kRatio[18] = {14.40, 12.29, 8.56, 7.30, 6.05, 5.16, 4.38, 3.74, 3.20, 2.73, 2.29, 1.95, 1.62, 1.38, 1.17, 1.00, 0.86, 0.73};
} // namespace

TEST_CASE("truck box: the lever, the range and the splitter make the eighteen gears", "[diesel][truckparts]")
{
    // every slot, the speeds matched (wCrank = wOut x the slot's ratio), the clutch in: each goes in at once
    Rig r;
    const double wOut = 20.0;
    int k = 0;
    for (int range : {0, 1})
        for (int lever : {5, 1, 2, 3, 4})
        {
            if (range == 1 && lever == 5)
                continue;
            for (int split : {0, 1})
            {
                const int expect = lever == 5 ? 1 + split : (range == 0 ? 1 + 2 * lever + split : 9 + 2 * lever + split);
                r.set("lever", 0);
                r.run(0.01);
                r.set("buttons", (split ? 16 : 0) + (range ? 32 : 0));
                r.set("wOut", wOut);
                r.set("wCrank", wOut * kRatio[expect - 1]);
                r.set("clutch", 0.0);
                r.set("lever", lever);
                r.run(0.02);
                CHECK(r.get("box.gear") == expect);
                ++k;
            }
        }
    CHECK(k == 18);
}

TEST_CASE("truck box: a gear floated in at matched revs goes in silently; 300 rpm off it grinds and stays out",
          "[diesel][truckparts][sound]")
{
    Rig r;
    // in 8 (low range, lever 3, high split) at 1500 rpm, the clutch in, then off the pedal to neutral
    const double w8 = 1500.0 * 0.10471976;
    const double wOut = w8 / kRatio[7];
    r.set("buttons", 16);
    r.set("wOut", wOut);
    r.set("wCrank", w8);
    r.set("lever", 3);
    r.run(0.05);
    REQUIRE(r.get("box.gear") == 8);
    r.set("pedal", 0.0);
    r.set("lever", 0);
    r.run(0.2);
    // up to 9 (lever 4, low split): its speed at this road speed is 1500 x 3.20 / 3.74 = 1283 rpm
    const double w9 = wOut * kRatio[8];
    r.set("buttons", 0);
    r.set("wCrank", w9 + 3.0);      // matched within the window (a driver's ear)
    r.set("lever", 4);
    r.run(0.5);
    CHECK(r.get("box.gear") == 9);
    CHECK(Rig::rms(r.g) < 1e-6);
    // the same shift with the revs left 300 rpm high: it grinds and does not go in
    r.set("lever", 0);
    r.run(0.2);
    r.set("wCrank", w9 + 300.0 * 0.10471976);
    r.set("lever", 4);
    r.run(0.5);
    std::printf("float in at matched revs: gear %.0f, silent; 300 rpm high: gear %.0f, grind rms %.3f, clash at %.0f Hz\n", 9.0,
                r.get("box.gear"), Rig::rms(r.g), r.get("box.clashHz"));
    CHECK(r.get("box.gear") == 0);
    CHECK(Rig::rms(r.g) > 0.02);
    CHECK(r.get("box.grinds") >= 1);
}

TEST_CASE("truck air: the governor cuts out at 8.3 bar with the dryer's purge; the spring brakes' release is heard",
          "[diesel][truckparts][sound]")
{
    Rig r;
    r.set("wCrank", 700.0 * 0.10471976);
    r.set("park", 1.0);
    double tCut = -1.0;
    for (int i = 0; i < 1200 && tCut < 0.0; ++i)
    {
        r.run(0.1);
        if (r.get("air.purges") >= 1)
            tCut = 0.1 * (i + 1);
    }
    std::printf("air: from 6 bar at idle the governor cuts out after %.1f s (at %.2f bar), purge rms %.3f\n", tCut,
                r.get("air.p") / 1e5, Rig::rms(r.a));
    CHECK(tCut > 10.0);
    CHECK(tCut < 120.0);
    CHECK(Rig::rms(r.a) > 0.01);
    r.run(3.0);
    r.run(1.0);
    CHECK(Rig::rms(r.a) < 1e-3);     // (the purge over: quiet again)
    r.set("park", 0.0);
    r.run(0.5);
    const double release = Rig::rms(r.a);
    std::printf("air: the spring brakes released: rms %.3f over 0.5 s\n", release);
    CHECK(release > 0.01);
}

#include "Diesel.h"

TEST_CASE("truck air in the engine: the tanks charge while it runs and the spring brakes' release is heard", "[diesel][truckparts][sound]")
{
    // spec.car.truckAir with card L's listener (sound.model 1): the parts at their places
    d2::Engine e("truck-15l-six");
    e.spec("car.truckAir", 1);
    e.spec("sound.model", 1);
    e.ctl().handbrake = 1.0f;
    e.start();
    const double p0 = e.get("air.p");
    REQUIRE(e.run(2.0));
    CHECK(e.get("air.p") > p0);
    e.recording = true;
    REQUIRE(e.run(0.5));
    // (the loudest 50 ms: the release is a short blast over the engine's idle)
    auto loudest = [&](const std::vector<float>& x) {
        double best = -200.0;
        const size_t w = static_cast<size_t>(0.05 * e.rate);
        for (size_t i = 0; i + w <= x.size(); i += w / 2)
            best = std::max(best, d2::rmsDb(std::vector<float>(x.begin() + static_cast<std::ptrdiff_t>(i), x.begin() + static_cast<std::ptrdiff_t>(i + w))));
        return best;
    };
    const double before = loudest(e.rec);
    e.rec.clear();
    e.ctl().handbrake = 0.0f;
    REQUIRE(e.run(0.5));
    const double after = loudest(e.rec);
    std::printf("truck air in the engine: tanks %.2f -> %.2f bar; idling %.1f dB, the spring brakes released %.1f dB\n", p0 / 1e5,
                e.get("air.p") / 1e5, before, after);
    CHECK(after > before + 2.0);
}
