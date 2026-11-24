// shell/audio/Wav.h: a stereo WAV file of 32-bit floats, written as the samples come (the offline render, the
// headless application; the recorder of slice 2 will write through it from its worker). Not for the audio thread:
// write() touches the disk.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace tee::audio
{
    class WavWriter
    {
    public:
        WavWriter() = default;
        ~WavWriter();                                   // close()
        WavWriter(const WavWriter&) = delete;
        WavWriter& operator=(const WavWriter&) = delete;

        // Starts the file (its folder must exist). False, and why, when it cannot be made.
        bool open(const std::string& path, int rate, std::string& why);
        // n frames of both channels. False once a write has failed.
        bool write(const float* left, const float* right, int n);
        // Writes the sizes into the header and closes. False when the file is not whole.
        bool close();

        std::uint64_t frames() const noexcept { return frames_; }

    private:
        std::FILE* file_ = nullptr;
        std::uint64_t frames_ = 0;
        int rate_ = 0;
        bool failed_ = false;
    };
}
