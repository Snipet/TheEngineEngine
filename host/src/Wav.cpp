// host/src/Wav.cpp: WAV files of 32-bit floats (Wav.h). Little-endian, as every system this runs on is.
#include "tee/Wav.h"

#include <cstdint>
#include <cstring>
#include <cmath>
#include <fstream>

namespace tee
{
    namespace
    {
        void put32(std::string& out, std::uint32_t v)
        {
            for (int k = 0; k < 4; ++k)
                out.push_back(static_cast<char>((v >> (8 * k)) & 0xFFu));
        }

        void put16(std::string& out, std::uint16_t v)
        {
            out.push_back(static_cast<char>(v & 0xFFu));
            out.push_back(static_cast<char>((v >> 8) & 0xFFu));
        }

        std::uint32_t get32(const unsigned char* at)
        {
            return static_cast<std::uint32_t>(at[0]) | static_cast<std::uint32_t>(at[1]) << 8
                   | static_cast<std::uint32_t>(at[2]) << 16 | static_cast<std::uint32_t>(at[3]) << 24;
        }

        std::uint16_t get16(const unsigned char* at)
        {
            return static_cast<std::uint16_t>(at[0] | (at[1] << 8));
        }
    } // namespace

    bool writeWav(const std::filesystem::path& file, const Audio& audio, std::string& error)
    {
        if (audio.left.size() != audio.right.size())
        {
            error = file.generic_string() + ": the two channels differ in length";
            return false;
        }
        const std::uint64_t dataBytes = static_cast<std::uint64_t>(audio.left.size()) * 8u;
        if (dataBytes > 0xFFFFFF00u)
        {
            error = file.generic_string() + ": too long for a WAV file (4 GB)";
            return false;
        }
        std::string head;
        head += "RIFF";
        put32(head, static_cast<std::uint32_t>(4 + 8 + 16 + 8 + 4 + 8 + dataBytes));
        head += "WAVEfmt ";
        put32(head, 16);
        put16(head, 3);                                                   // IEEE float
        put16(head, 2);                                                   // channels
        put32(head, static_cast<std::uint32_t>(audio.sampleRate + 0.5));
        put32(head, static_cast<std::uint32_t>(audio.sampleRate + 0.5) * 8u);   // bytes a second
        put16(head, 8);                                                   // bytes a frame
        put16(head, 32);                                                  // bits a sample
        head += "fact";                                                   // a float file says its frames
        put32(head, 4);
        put32(head, static_cast<std::uint32_t>(audio.left.size()));
        head += "data";
        put32(head, static_cast<std::uint32_t>(dataBytes));

        std::error_code ec;
        if (file.has_parent_path())
            std::filesystem::create_directories(file.parent_path(), ec);
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out)
        {
            error = file.generic_string() + ": cannot be written";
            return false;
        }
        out.write(head.data(), static_cast<std::streamsize>(head.size()));
        std::vector<float> frames(audio.left.size() * 2);
        for (std::size_t i = 0; i < audio.left.size(); ++i)
        {
            frames[2 * i] = audio.left[i];
            frames[2 * i + 1] = audio.right[i];
        }
        out.write(reinterpret_cast<const char*>(frames.data()), static_cast<std::streamsize>(frames.size() * sizeof(float)));
        out.flush();
        if (!out)
        {
            error = file.generic_string() + ": could not be written whole";
            return false;
        }
        return true;
    }

    bool writeWav24(const std::filesystem::path& file, const Audio& audio, std::string& error)
    {
        if (audio.left.size() != audio.right.size())
        {
            error = file.generic_string() + ": the two channels differ in length";
            return false;
        }
        const std::uint64_t dataBytes = static_cast<std::uint64_t>(audio.left.size()) * 6u;
        if (dataBytes > 0xFFFFFF00u)
        {
            error = file.generic_string() + ": too long for a WAV file (4 GB)";
            return false;
        }
        std::string head;
        head += "RIFF";
        put32(head, static_cast<std::uint32_t>(4 + 8 + 16 + 8 + dataBytes));
        head += "WAVEfmt ";
        put32(head, 16);
        put16(head, 1);                                                   // integer PCM
        put16(head, 2);
        put32(head, static_cast<std::uint32_t>(audio.sampleRate + 0.5));
        put32(head, static_cast<std::uint32_t>(audio.sampleRate + 0.5) * 6u);
        put16(head, 6);
        put16(head, 24);
        head += "data";
        put32(head, static_cast<std::uint32_t>(dataBytes));
        std::string body;
        body.reserve(static_cast<std::size_t>(dataBytes));
        const auto put24 = [&](float v) {
            const double c = std::isfinite(v) ? std::fmax(-1.0, std::fmin(1.0, static_cast<double>(v))) : 0.0;
            const std::int32_t q = static_cast<std::int32_t>(std::lround(c * 8388607.0));
            const std::uint32_t u = static_cast<std::uint32_t>(q);
            body += static_cast<char>(u & 0xffu);
            body += static_cast<char>((u >> 8) & 0xffu);
            body += static_cast<char>((u >> 16) & 0xffu);
        };
        for (std::size_t i = 0; i < audio.left.size(); ++i)
        {
            put24(audio.left[i]);
            put24(audio.right[i]);
        }
        std::error_code ec;
        if (file.has_parent_path())
            std::filesystem::create_directories(file.parent_path(), ec);
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out.write(head.data(), static_cast<std::streamsize>(head.size()));
        out.write(body.data(), static_cast<std::streamsize>(body.size()));
        out.flush();
        if (!out)
        {
            error = file.generic_string() + ": cannot be written";
            return false;
        }
        return true;
    }

    bool readWav(const std::filesystem::path& file, Audio& audio, std::string& error)
    {
        std::ifstream in(file, std::ios::binary);
        if (!in)
        {
            error = file.generic_string() + ": cannot be read";
            return false;
        }
        const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const auto* data = reinterpret_cast<const unsigned char*>(bytes.data());
        const std::size_t size = bytes.size();
        if (size < 12 || std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0)
        {
            error = file.generic_string() + ": is not a WAV file";
            return false;
        }
        int channels = 0;
        bool floats = false;
        audio = Audio{};
        std::size_t at = 12;
        while (at + 8 <= size)
        {
            const std::size_t length = get32(data + at + 4);
            const unsigned char* body = data + at + 8;
            if (length > size - at - 8)
                break;
            if (std::memcmp(data + at, "fmt ", 4) == 0 && length >= 16)
            {
                channels = get16(body + 2);
                audio.sampleRate = static_cast<double>(get32(body + 4));
                floats = get16(body) == 3 && get16(body + 14) == 32;
            }
            else if (std::memcmp(data + at, "data", 4) == 0)
            {
                if (!floats || (channels != 1 && channels != 2))
                {
                    error = file.generic_string() + ": is not a WAV of 32-bit floats, mono or stereo";
                    return false;
                }
                const std::size_t frames = length / (4u * static_cast<std::size_t>(channels));
                audio.left.resize(frames);
                audio.right.resize(frames);
                for (std::size_t i = 0; i < frames; ++i)
                {
                    std::memcpy(&audio.left[i], body + i * 4u * static_cast<std::size_t>(channels), 4);
                    if (channels == 2)
                        std::memcpy(&audio.right[i], body + i * 8u + 4u, 4);
                    else
                        audio.right[i] = audio.left[i];
                }
                return true;
            }
            at += 8 + length + (length & 1u);
        }
        error = file.generic_string() + ": has no samples";
        return false;
    }
} // namespace tee
