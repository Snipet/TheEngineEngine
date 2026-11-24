// tests/ui/Png.h: a PNG file from FunkGui's software rasteriser's picture, for the kit's tests. FunkGui's own encoder
// is JUCE's, so its JUCE-free build writes none (funkgui::writePng returns false there); this one is ours until
// FunkGui has a JUCE-free one (card A, stage 2). It is the simplest valid PNG: 8-bit RGBA, no filter, the pixels in
// stored (uncompressed) deflate blocks. The files are large and only ever looked at; they go under out/.
#pragma once

#include <funkgui/canvas/SoftRaster.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace tee::test
{
    inline uint32_t crc32(const uint8_t* data, std::size_t n, uint32_t crc = 0)
    {
        static uint32_t table[256];
        static bool made = false;
        if (!made)
        {
            for (uint32_t i = 0; i < 256; ++i)
            {
                uint32_t c = i;
                for (int k = 0; k < 8; ++k)
                    c = (c & 1u) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
                table[i] = c;
            }
            made = true;
        }
        crc = ~crc;
        for (std::size_t i = 0; i < n; ++i)
            crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
        return ~crc;
    }

    inline bool writePng(const funkgui::Image& img, const std::string& path)
    {
        if (img.w <= 0 || img.h <= 0 || img.rgba.size() != static_cast<std::size_t>(img.w) * static_cast<std::size_t>(img.h) * 4)
            return false;
        const auto be32 = [](std::vector<uint8_t>& out, uint32_t v) {
            for (int s = 24; s >= 0; s -= 8)
                out.push_back(static_cast<uint8_t>(v >> s));
        };
        // The scanlines, each with its filter byte (0: none).
        std::vector<uint8_t> raw;
        const std::size_t row = static_cast<std::size_t>(img.w) * 4;
        raw.reserve((row + 1) * static_cast<std::size_t>(img.h));
        for (int y = 0; y < img.h; ++y)
        {
            raw.push_back(0);
            raw.insert(raw.end(), img.rgba.begin() + static_cast<std::ptrdiff_t>(row * static_cast<std::size_t>(y)),
                       img.rgba.begin() + static_cast<std::ptrdiff_t>(row * static_cast<std::size_t>(y + 1)));
        }
        // zlib: a header, stored blocks of at most 65535 bytes, the Adler-32 of the data.
        std::vector<uint8_t> z{ 0x78, 0x01 };
        uint32_t a = 1, b = 0;
        for (std::size_t at = 0; at < raw.size();)
        {
            const std::size_t n = std::min<std::size_t>(65535, raw.size() - at);
            z.push_back(at + n == raw.size() ? 1 : 0);
            z.push_back(static_cast<uint8_t>(n & 0xFF));
            z.push_back(static_cast<uint8_t>(n >> 8));
            z.push_back(static_cast<uint8_t>(~n & 0xFF));
            z.push_back(static_cast<uint8_t>((~n >> 8) & 0xFF));
            for (std::size_t i = 0; i < n; ++i)
            {
                a = (a + raw[at + i]) % 65521u;
                b = (b + a) % 65521u;
            }
            z.insert(z.end(), raw.begin() + static_cast<std::ptrdiff_t>(at), raw.begin() + static_cast<std::ptrdiff_t>(at + n));
            at += n;
        }
        be32(z, (b << 16) | a);

        std::vector<uint8_t> file{ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
        const auto chunk = [&](const char* type, const std::vector<uint8_t>& body) {
            be32(file, static_cast<uint32_t>(body.size()));
            const std::size_t from = file.size();
            file.insert(file.end(), type, type + 4);
            file.insert(file.end(), body.begin(), body.end());
            be32(file, crc32(file.data() + from, file.size() - from));
        };
        std::vector<uint8_t> head;
        be32(head, static_cast<uint32_t>(img.w));
        be32(head, static_cast<uint32_t>(img.h));
        head.insert(head.end(), { 8, 6, 0, 0, 0 });      // 8 bits a channel, RGBA, deflate, no filter method, no interlace
        chunk("IHDR", head);
        chunk("IDAT", z);
        chunk("IEND", {});
        std::FILE* f = std::fopen(path.c_str(), "wb");
        if (f == nullptr)
            return false;
        const bool written = std::fwrite(file.data(), 1, file.size(), f) == file.size();
        return std::fclose(f) == 0 && written;
    }
}
