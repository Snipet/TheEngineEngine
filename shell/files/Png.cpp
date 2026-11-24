// shell/files/Png.cpp: Png.h. The file is the signature, IHDR, one IDAT and IEND. The IDAT is a zlib stream (RFC
// 1950) of stored deflate blocks (RFC 1951, section 3.2.4: at most 65535 bytes each), over the rows, each with filter
// byte 0 (none) before its pixels.
#include "files/Png.h"

#include <array>
#include <cstdio>
#include <cstring>

namespace tee::files
{
    namespace
    {
        std::uint32_t crc32(const std::uint8_t* data, std::size_t n, std::uint32_t crc = 0) noexcept
        {
            static const auto table = [] {
                std::array<std::uint32_t, 256> t{};
                for (std::uint32_t i = 0; i < 256; ++i)
                {
                    std::uint32_t c = i;
                    for (int k = 0; k < 8; ++k)
                        c = (c & 1u) != 0 ? 0xedb88320u ^ (c >> 1) : c >> 1;
                    t[i] = c;
                }
                return t;
            }();
            crc = ~crc;
            for (std::size_t i = 0; i < n; ++i)
                crc = table[(crc ^ data[i]) & 0xffu] ^ (crc >> 8);
            return ~crc;
        }

        void put32be(std::vector<std::uint8_t>& out, std::uint32_t v)
        {
            out.push_back(static_cast<std::uint8_t>(v >> 24));
            out.push_back(static_cast<std::uint8_t>(v >> 16));
            out.push_back(static_cast<std::uint8_t>(v >> 8));
            out.push_back(static_cast<std::uint8_t>(v));
        }

        void chunk(std::vector<std::uint8_t>& out, const char type[4], const std::vector<std::uint8_t>& body)
        {
            put32be(out, static_cast<std::uint32_t>(body.size()));
            const std::size_t at = out.size();
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), body.begin(), body.end());
            put32be(out, crc32(out.data() + at, out.size() - at));
        }
    }

    std::vector<std::uint8_t> encodePng(const std::uint8_t* rgba, int w, int h)
    {
        std::vector<std::uint8_t> out;
        if (rgba == nullptr || w <= 0 || h <= 0)
            return out;
        const auto width = static_cast<std::size_t>(w), height = static_cast<std::size_t>(h);
        const std::size_t row = width * 4 + 1;

        // the rows as PNG wants them: a filter byte, then the pixels
        std::vector<std::uint8_t> raw(row * height);
        for (std::size_t y = 0; y < height; ++y)
        {
            raw[y * row] = 0;
            std::memcpy(raw.data() + y * row + 1, rgba + y * width * 4, width * 4);
        }

        // zlib: a header, stored blocks, Adler-32 of the raw bytes
        std::vector<std::uint8_t> z;
        z.reserve(raw.size() + raw.size() / 65535 * 5 + 16);
        z.push_back(0x78);
        z.push_back(0x01);
        std::uint32_t a = 1, b = 0;
        std::size_t at = 0;
        while (at < raw.size())
        {
            const std::size_t n = raw.size() - at < 65535 ? raw.size() - at : 65535;
            z.push_back(at + n == raw.size() ? 1 : 0);             // the last block's flag; type 00, stored
            z.push_back(static_cast<std::uint8_t>(n & 0xffu));
            z.push_back(static_cast<std::uint8_t>(n >> 8));
            z.push_back(static_cast<std::uint8_t>(~n & 0xffu));
            z.push_back(static_cast<std::uint8_t>((~n >> 8) & 0xffu));
            z.insert(z.end(), raw.begin() + static_cast<std::ptrdiff_t>(at),
                     raw.begin() + static_cast<std::ptrdiff_t>(at + n));
            for (std::size_t i = 0; i < n; ++i)
            {
                a = (a + raw[at + i]) % 65521u;
                b = (b + a) % 65521u;
            }
            at += n;
        }
        put32be(z, (b << 16) | a);

        static const std::uint8_t signature[8] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };
        out.reserve(z.size() + 64);
        out.insert(out.end(), signature, signature + 8);
        std::vector<std::uint8_t> head;
        put32be(head, static_cast<std::uint32_t>(w));
        put32be(head, static_cast<std::uint32_t>(h));
        head.push_back(8);      // bits a channel
        head.push_back(6);      // RGBA
        head.push_back(0);      // deflate
        head.push_back(0);      // the five filters
        head.push_back(0);      // not interlaced
        chunk(out, "IHDR", head);
        chunk(out, "IDAT", z);
        chunk(out, "IEND", {});
        return out;
    }

    bool writePng(const std::string& path, const std::uint8_t* rgba, int w, int h, std::string& why)
    {
        const std::vector<std::uint8_t> bytes = encodePng(rgba, w, h);
        if (bytes.empty())
        {
            why = "no picture to write to " + path;
            return false;
        }
        std::FILE* f = std::fopen(path.c_str(), "wb");
        if (f == nullptr)
        {
            why = "cannot write " + path;
            return false;
        }
        const bool wrote = std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
        const bool closed = std::fclose(f) == 0;
        if (!wrote || !closed)
        {
            why = "cannot write " + path;
            return false;
        }
        return true;
    }
}
