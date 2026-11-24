// tests/shell/FilesTest.cpp: the PNG and WAV writers, the script reader and the command line.
#include "Options.h"
#include "Script.h"
#include "audio/Wav.h"
#include "files/Png.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace tee;

namespace
{
    std::uint32_t be32(const std::uint8_t* p)
    {
        return (std::uint32_t{ p[0] } << 24) | (std::uint32_t{ p[1] } << 16) | (std::uint32_t{ p[2] } << 8) | p[3];
    }

    std::uint32_t le32(const std::uint8_t* p)
    {
        return (std::uint32_t{ p[3] } << 24) | (std::uint32_t{ p[2] } << 16) | (std::uint32_t{ p[1] } << 8) | p[0];
    }

    // CRC-32 a bit at a time: another way than the writer's table, so the two can disagree
    std::uint32_t crc(const std::uint8_t* p, std::size_t n)
    {
        std::uint32_t c = 0xffffffffu;
        for (std::size_t i = 0; i < n; ++i)
        {
            c ^= p[i];
            for (int k = 0; k < 8; ++k)
                c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
        }
        return ~c;
    }

    std::vector<std::uint8_t> readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
    }

    std::filesystem::path outDir()
    {
        const std::filesystem::path dir = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "shell";
        std::filesystem::create_directories(dir);
        return dir;
    }
}

TEST_CASE("shell: a PNG reads back as the picture that was written", "[shell][files]")
{
    // a picture large enough to need more than one stored block (65535 bytes each)
    const int w = 301, h = 97;
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(w) * h * 4);
    for (std::size_t i = 0; i < rgba.size(); ++i)
        rgba[i] = static_cast<std::uint8_t>((i * 2654435761u) >> 13);
    const std::vector<std::uint8_t> png = files::encodePng(rgba.data(), w, h);
    REQUIRE(png.size() > 8 + 25 + 12);
    static const std::uint8_t signature[8] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };
    REQUIRE(std::memcmp(png.data(), signature, 8) == 0);

    // the chunks: each one's CRC, IHDR's fields, the IDAT's zlib stream
    std::size_t at = 8;
    std::vector<std::uint8_t> zlib;
    std::vector<std::string> types;
    while (at + 12 <= png.size())
    {
        const std::uint32_t length = be32(png.data() + at);
        REQUIRE(at + 12 + length <= png.size());
        const std::string type(reinterpret_cast<const char*>(png.data() + at + 4), 4);
        CHECK(crc(png.data() + at + 4, 4 + length) == be32(png.data() + at + 8 + length));
        if (type == "IHDR")
        {
            REQUIRE(length == 13);
            CHECK(be32(png.data() + at + 8) == static_cast<std::uint32_t>(w));
            CHECK(be32(png.data() + at + 12) == static_cast<std::uint32_t>(h));
            CHECK(png[at + 16] == 8);       // bits a channel
            CHECK(png[at + 17] == 6);       // RGBA
        }
        if (type == "IDAT")
            zlib.assign(png.begin() + static_cast<std::ptrdiff_t>(at + 8),
                        png.begin() + static_cast<std::ptrdiff_t>(at + 8 + length));
        types.push_back(type);
        at += 12 + length;
    }
    CHECK(at == png.size());
    REQUIRE(types == std::vector<std::string>{ "IHDR", "IDAT", "IEND" });

    // inflate the stored blocks and check the Adler-32
    REQUIRE(zlib.size() > 6);
    CHECK(((zlib[0] << 8) | zlib[1]) % 31 == 0);            // the zlib header's check
    CHECK((zlib[0] & 0x0f) == 8);                           // deflate
    std::vector<std::uint8_t> raw;
    std::size_t z = 2;
    bool last = false;
    while (!last)
    {
        REQUIRE(z + 5 <= zlib.size());
        last = (zlib[z] & 1) != 0;
        REQUIRE((zlib[z] & 6) == 0);                        // stored
        const unsigned len = zlib[z + 1] | (zlib[z + 2] << 8);
        const unsigned nlen = zlib[z + 3] | (zlib[z + 4] << 8);
        REQUIRE((len ^ nlen) == 0xffffu);
        REQUIRE(z + 5 + len + 4 <= zlib.size());
        raw.insert(raw.end(), zlib.begin() + static_cast<std::ptrdiff_t>(z + 5),
                   zlib.begin() + static_cast<std::ptrdiff_t>(z + 5 + len));
        z += 5 + len;
    }
    REQUIRE(z + 4 == zlib.size());
    std::uint32_t a = 1, b = 0;
    for (const std::uint8_t byte : raw)
    {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    CHECK(((b << 16) | a) == be32(zlib.data() + z));
    // the rows: a filter byte 0, then the pixels as given
    REQUIRE(raw.size() == static_cast<std::size_t>(h) * (static_cast<std::size_t>(w) * 4 + 1));
    bool same = true;
    for (int y = 0; y < h && same; ++y)
    {
        const std::size_t row = static_cast<std::size_t>(y) * (static_cast<std::size_t>(w) * 4 + 1);
        same = raw[row] == 0
               && std::memcmp(raw.data() + row + 1, rgba.data() + static_cast<std::size_t>(y) * w * 4,
                              static_cast<std::size_t>(w) * 4) == 0;
    }
    CHECK(same);

    CHECK(files::encodePng(rgba.data(), 0, 5).empty());
    std::string why;
    CHECK_FALSE(files::writePng((outDir() / "no-such-folder" / "x.png").string(), rgba.data(), w, h, why));
    CHECK_FALSE(why.empty());
    CHECK(files::writePng((outDir() / "test-noise.png").string(), rgba.data(), w, h, why));
    CHECK(readFile(outDir() / "test-noise.png") == png);
}

TEST_CASE("shell: a WAV file's header says what was written", "[shell][files]")
{
    const std::filesystem::path path = outDir() / "test-ramp.wav";
    std::vector<float> l(1000), r(1000);
    for (std::size_t i = 0; i < l.size(); ++i)
    {
        l[i] = static_cast<float>(i) / 1000.0f;
        r[i] = -l[i];
    }
    {
        audio::WavWriter wav;
        std::string why;
        REQUIRE(wav.open(path.string(), 44100, why));
        CHECK(wav.write(l.data(), r.data(), 600));
        CHECK(wav.write(l.data() + 600, r.data() + 600, 400));
        CHECK(wav.frames() == 1000);
        CHECK(wav.close());
    }
    const std::vector<std::uint8_t> f = readFile(path);
    REQUIRE(f.size() == 58 + 1000 * 8);
    CHECK(std::memcmp(f.data(), "RIFF", 4) == 0);
    CHECK(le32(f.data() + 4) == f.size() - 8);
    CHECK(std::memcmp(f.data() + 8, "WAVEfmt ", 8) == 0);
    CHECK((f[20] | (f[21] << 8)) == 3);                     // floats
    CHECK((f[22] | (f[23] << 8)) == 2);                     // stereo
    CHECK(le32(f.data() + 24) == 44100);
    CHECK((f[34] | (f[35] << 8)) == 32);
    CHECK(std::memcmp(f.data() + 38, "fact", 4) == 0);
    CHECK(le32(f.data() + 46) == 1000);
    CHECK(std::memcmp(f.data() + 50, "data", 4) == 0);
    CHECK(le32(f.data() + 54) == 8000);
    float sample = 0.0f;
    std::memcpy(&sample, f.data() + 58 + 8 * 500, 4);
    CHECK(sample == l[500]);
    std::memcpy(&sample, f.data() + 58 + 8 * 500 + 4, 4);
    CHECK(sample == r[500]);

    audio::WavWriter none;
    std::string why;
    CHECK_FALSE(none.open((outDir() / "no-such-folder" / "x.wav").string(), 48000, why));
    CHECK_FALSE(why.empty());
}

TEST_CASE("shell: a script is read by time, and a line that is none is refused with its number", "[shell][app]")
{
    app::Script s;
    std::string why;
    REQUIRE(app::parseScript("# a comment\n"
                             "2.0 key w up\n"
                             "0.5 key w down   # the throttle\n"
                             "1.0 tap e 0.3\n"
                             "1.1 set size 6.2\n"
                             "1.2 pointer click 40 300\n"
                             "1.3 wheel 40 300 -1\n"
                             "1.4 shot out/a.png\n"
                             "1.5 expect engine.state == 2\n"
                             "1.6 expect param size >= 6\n"
                             "3.0 end\n"
                             "9.0 key w down\n",
                             s, why));
    CHECK(s.end == 3.0);
    REQUIRE(s.events.size() == 11);
    CHECK(s.events[0].time == 0.5);
    CHECK(s.events[0].kind == app::ScriptEvent::Kind::key);
    CHECK(s.events[0].name == "w");
    CHECK(s.events[0].a == 1.0);
    CHECK(s.events[1].time == 1.0);                         // the tap's down
    CHECK(s.events[3].time == 1.2);
    CHECK(s.events[2].kind == app::ScriptEvent::Kind::set);
    CHECK(s.events[2].a == 6.2);
    CHECK(s.events[4].time == 1.3);                         // the tap's up, 0.3 s after its down, in its place
    CHECK(s.events[4].kind == app::ScriptEvent::Kind::key);
    CHECK(s.events[4].a == 0.0);
    CHECK(s.events[8].name == "param size");
    CHECK(s.events[8].op == ">=");
    CHECK(s.events.back().kind == app::ScriptEvent::Kind::end);

    CHECK_FALSE(app::parseScript("0.5 key w down\n1.0 dance\n", s, why));
    CHECK(why.find("line 2") != std::string::npos);
    CHECK_FALSE(app::parseScript("soon key w down\n", s, why));
    CHECK_FALSE(app::parseScript("1 expect rpm ~ 3\n", s, why));

    const app::Script d = app::defaultScript(10.0);
    CHECK(d.end == 10.0);
    CHECK(d.events.size() == 6);
}

TEST_CASE("shell: the command line", "[shell][app]")
{
    const auto parse = [](std::vector<std::string> words, app::Options& o, std::string& why) {
        std::vector<char*> argv;
        static std::string program = "tee";
        argv.push_back(program.data());
        for (std::string& w : words)
            argv.push_back(w.data());
        return app::parseOptions(static_cast<int>(argv.size()), argv.data(), o, why);
    };
    app::Options o;
    std::string why;
    REQUIRE(parse({ "--seconds", "2.5", "--wav", "a.wav", "--frames", "300", "--shot", "a.png", "--preset", "v8", "--set",
                    "size=6.2", "--set", "drive=0", "--engine", "tone", "--rate", "44100", "--block", "64", "--info" },
                  o, why));
    CHECK(o.seconds == 2.5);
    CHECK(o.wav == "a.wav");
    CHECK(o.frames == 300);
    CHECK(o.shot == "a.png");
    CHECK(o.preset == "v8");
    REQUIRE(o.sets.size() == 2);
    CHECK(o.sets[0].first == "size");
    CHECK(o.sets[0].second == 6.2);
    CHECK(o.engine == "tone");
    CHECK(o.rate == 44100);
    CHECK(o.block == 64);
    CHECK(o.info);

    app::Options bad;
    CHECK_FALSE(parse({ "--frames" }, bad, why));
    CHECK_FALSE(parse({ "--frames", "many" }, bad, why));
    CHECK_FALSE(parse({ "--set", "size" }, bad, why));
    CHECK_FALSE(parse({ "--engine", "v12" }, bad, why));
    CHECK_FALSE(parse({ "--loud" }, bad, why));
    CHECK(why.find("--loud") != std::string::npos);
    CHECK_FALSE(app::usage("tee").empty());
}
