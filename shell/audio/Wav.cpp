// shell/audio/Wav.cpp: Wav.h. The header is the 58-byte one WAVE_FORMAT_IEEE_FLOAT wants (a fact chunk and an
// 18-byte fmt), little-endian whatever the machine.
#include "audio/Wav.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace tee::audio
{
    namespace
    {
        void put32(unsigned char* at, std::uint32_t v) noexcept
        {
            at[0] = static_cast<unsigned char>(v & 0xffu);
            at[1] = static_cast<unsigned char>((v >> 8) & 0xffu);
            at[2] = static_cast<unsigned char>((v >> 16) & 0xffu);
            at[3] = static_cast<unsigned char>((v >> 24) & 0xffu);
        }

        void put16(unsigned char* at, std::uint16_t v) noexcept
        {
            at[0] = static_cast<unsigned char>(v & 0xffu);
            at[1] = static_cast<unsigned char>((v >> 8) & 0xffu);
        }

        constexpr std::size_t kHeader = 58;

        // The header for `frames` stereo frames (0 while the file is being written).
        std::array<unsigned char, kHeader> header(int rate, std::uint64_t frames) noexcept
        {
            std::array<unsigned char, kHeader> h{};
            const std::uint64_t bytes = frames * 8u;
            const std::uint32_t data = bytes > 0xffffff00u ? 0xffffff00u : static_cast<std::uint32_t>(bytes);
            std::memcpy(h.data(), "RIFF", 4);
            put32(h.data() + 4, data + static_cast<std::uint32_t>(kHeader) - 8u);
            std::memcpy(h.data() + 8, "WAVEfmt ", 8);
            put32(h.data() + 16, 18);                                    // the fmt chunk's size
            put16(h.data() + 20, 3);                                     // WAVE_FORMAT_IEEE_FLOAT
            put16(h.data() + 22, 2);                                     // channels
            put32(h.data() + 24, static_cast<std::uint32_t>(rate));
            put32(h.data() + 28, static_cast<std::uint32_t>(rate) * 8u); // bytes a second
            put16(h.data() + 32, 8);                                     // bytes a frame
            put16(h.data() + 34, 32);                                    // bits a sample
            put16(h.data() + 36, 0);                                     // no extension
            std::memcpy(h.data() + 38, "fact", 4);
            put32(h.data() + 42, 4);
            put32(h.data() + 46, static_cast<std::uint32_t>(std::min<std::uint64_t>(frames, 0xffffffffu)));
            std::memcpy(h.data() + 50, "data", 4);
            put32(h.data() + 54, data);
            return h;
        }
    }

    WavWriter::~WavWriter()
    {
        close();
    }

    bool WavWriter::open(const std::string& path, int rate, std::string& why)
    {
        close();
        frames_ = 0;
        failed_ = false;
        rate_ = rate;
        file_ = std::fopen(path.c_str(), "wb");
        if (file_ == nullptr)
        {
            why = "cannot write " + path;
            return false;
        }
        const auto h = header(rate, 0);
        if (std::fwrite(h.data(), 1, h.size(), file_) != h.size())
        {
            why = "cannot write " + path;
            std::fclose(file_);
            file_ = nullptr;
            return false;
        }
        return true;
    }

    bool WavWriter::write(const float* left, const float* right, int n)
    {
        if (file_ == nullptr || failed_ || n <= 0)
            return file_ != nullptr && !failed_;
        unsigned char chunk[512 * 8];
        int done = 0;
        while (done < n)
        {
            const int m = std::min(n - done, 512);
            for (int i = 0; i < m; ++i)
            {
                std::uint32_t l = 0, r = 0;
                std::memcpy(&l, left + done + i, 4);
                std::memcpy(&r, right + done + i, 4);
                put32(chunk + 8 * i, l);
                put32(chunk + 8 * i + 4, r);
            }
            const auto bytes = static_cast<std::size_t>(m) * 8u;
            if (std::fwrite(chunk, 1, bytes, file_) != bytes)
            {
                failed_ = true;
                return false;
            }
            done += m;
        }
        frames_ += static_cast<std::uint64_t>(n);
        return true;
    }

    bool WavWriter::close()
    {
        if (file_ == nullptr)
            return !failed_;
        const auto h = header(rate_, frames_);
        bool ok = !failed_;
        ok = std::fseek(file_, 0, SEEK_SET) == 0 && ok;
        ok = std::fwrite(h.data(), 1, h.size(), file_) == h.size() && ok;
        ok = std::fclose(file_) == 0 && ok;
        file_ = nullptr;
        failed_ = !ok;
        return ok;
    }
}
