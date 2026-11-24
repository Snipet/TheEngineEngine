// host/include/tee/Wav.h: WAV files of 32-bit floats, written and read whole. What tee-render writes and what a test
// or a tool reads back: the samples as the simulator made them, with no dither and no conversion.
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace tee
{
    struct Audio
    {
        double sampleRate = 48000.0;
        std::vector<float> left;
        std::vector<float> right;   // as long as left

        std::size_t frames() const noexcept { return left.size(); }
        double seconds() const noexcept { return sampleRate > 0.0 ? static_cast<double>(left.size()) / sampleRate : 0.0; }
    };

    // Writes a stereo WAV of IEEE floats (format 3, 32 bits), making the file's directory if it is not there. False,
    // with `error` set, when it cannot be written.
    bool writeWav(const std::filesystem::path& file, const Audio& audio, std::string& error);

    // Reads a WAV of 32-bit floats, mono (both channels the same) or stereo. False, with `error` set, for a file that
    // is not one.
    bool readWav(const std::filesystem::path& file, Audio& audio, std::string& error);
} // namespace tee
