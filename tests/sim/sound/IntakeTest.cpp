// tests/sim/sound/IntakeTest.cpp: with intake.model 1 the intake's voice is card E's own radiated sound (Intake::sound:
// its air box's snorkel, or individual throttle bodies' trumpets), not the reference's box filter. Heard here as the
// difference between a run with the intake's level up and the same run with it at 0 (the exhaust and everything else
// the same to the bit), at the firing frequency's harmonics.
#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace
{
    constexpr double kRate = 48000.0;
    constexpr int kBlock = 512;
    constexpr double kTwoPi = 6.28318530717958647692;

    struct Take
    {
        std::vector<float> out;
        double soundRms = 0.0;      // card E's own measure: intk.soundMs, its mean over the take
        double mouthRms = 0.0;      // the intake's voice as Extras makes it: ex.mouthMs
    };

    // The default build, intake.model 1 with trumpets, throttle kind `kind` (0 a plenum's, 1 individual throttle
    // bodies), the intake's level `level`, the whole at -24 dB (the trumpets, 31 dB over a snorkel, would otherwise drive
    // the listener's soft ceiling and the difference would not be the intake's alone): started, warm, held at rpm wide open on the engine dyno; 0.5 s kept
    Take take(int kind, double level, double rpm)
    {
        tee::EnginePlayer::Options options;
        options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = kRate;
        std::string errors;
        auto player = tee::EnginePlayer::create(options, errors);
        INFO(errors);
        REQUIRE(player != nullptr);
        tee::EngineBuild b;
        std::string warnings;
        REQUIRE(tee::loadBuild(*player, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "default.json", b,
                               errors, warnings));
        tee::applyBuild(*player, b);
        for (const auto& [path, value] : std::vector<std::pair<std::string, double>>{
                 {"intake.model", 1.0}, {"intake.throttleKind", double(kind)}, {"intake.trumpets", 1.0},
                 {"sound.intake", level}, {"sound.view", 3.0}, {"sound.level", -24.0}})
        {
            INFO(path);
            REQUIRE(player->setSpec(path, value, errors));
        }
        std::vector<float> l(kBlock), r(kBlock);
        auto play = [&](double s, auto each) {
            for (int i = 0; i < int(s * kRate / kBlock + 0.5); ++i)
            {
                player->process(l.data(), r.data(), kBlock);
                each();
            }
        };
        auto& c = player->controls();
        c.ignition = tee::EnginePlayer::kIgnitionStart;
        play(1.0, [] {});
        c.ignition = tee::EnginePlayer::kIgnitionOn;
        play(1.0, [] {});
        c.mode = 2;
        c.dynoSpeed = float(rpm * kTwoPi / 60.0);
        c.throttle = 1.0f;
        play(2.0, [] {});
        Take t;
        double s = 0.0, sm = 0.0;
        int n = 0;
        play(0.5, [&] {
            t.out.insert(t.out.end(), l.begin(), l.end());
            double v = 0.0;
            std::string e;
            REQUIRE(player->read("intk.soundMs", v, e));
            s += v;
            REQUIRE(player->read("ex.mouthMs", v, e));
            sm += v;
            n += 1;
        });
        t.soundRms = std::sqrt(s / n);
        t.mouthRms = std::sqrt(sm / n);
        return t;
    }

    // the power at f in x (a Hann window's)
    double power(const std::vector<float>& x, double f)
    {
        double re = 0.0, im = 0.0;
        const std::size_t n = x.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos(kTwoPi * double(i) / double(n));
            re += w * x[i] * std::cos(kTwoPi * f * double(i) / kRate);
            im += w * x[i] * std::sin(kTwoPi * f * double(i) / kRate);
        }
        return re * re + im * im;
    }
}

TEST_CASE("intake: with intake.model 1 the trumpets of individual throttle bodies are heard over a plenum's snorkel",
          "[sound][intake][nightly]")
{
    const double rpm = 4000.0;
    const double fire = rpm / 60.0 * 2.0;       // the default build's four: two firings a turn
    double sum[2] = {0.0, 0.0};
    double eDb[2] = {0.0, 0.0};
    double mDb[2] = {0.0, 0.0};
    for (int kind : {0, 1})
    {
        const Take up = take(kind, 1.0, rpm), none = take(kind, 0.0, rpm);
        std::vector<float> d(up.out.size());
        for (std::size_t i = 0; i < d.size(); ++i)
            d[i] = up.out[i] - none.out[i];
        for (int h = 1; h <= 6; ++h)
            sum[kind] += power(d, h * fire);
        double rd = 0.0, ru = 0.0, pk = 0.0;
        for (std::size_t i = 0; i < d.size(); ++i)
        {
            rd += double(d[i]) * d[i];
            ru += double(up.out[i]) * up.out[i];
            pk = std::max(pk, std::fabs(double(up.out[i])));
        }
        std::printf("[m] kind %d: difference rms %.5f, output rms %.4f, peak %.3f, soundRms %.5f\n", kind,
                    std::sqrt(rd / d.size()), std::sqrt(ru / d.size()), pk, up.soundRms);
        eDb[kind] = up.soundRms;
        mDb[kind] = up.mouthRms;
    }
    const double heard = 10.0 * std::log10(sum[1] / sum[0]);
    const double radiated = 20.0 * std::log10(eDb[1] / eDb[0]);
    const double voice = 20.0 * std::log10(mDb[1] / mDb[0]);
    std::printf("[m] at %.0f rpm wide open, the intake's voice at the firing's first six harmonics: ITBs %+.1f dB over the "
                "plenum's snorkel; the intake's voice (ex.mouth) %+.1f dB; card E's own radiated measure %+.1f dB\n", rpm, heard,
                voice, radiated);
    // the voice is card E's sound itself: its level against the snorkel's as E measures it
    CHECK(std::fabs(voice - radiated) < 1.0);
    // and in the mix, through the listener's compressor and soft ceiling (which hold the trumpets back), still well up
    CHECK(heard > 10.0);
}
