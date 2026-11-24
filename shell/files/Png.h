// shell/files/Png.h: a picture written as a PNG file, for the headless application's frames and the window's
// screenshots. RGBA, 8 bits a channel, straight alpha, rows from the top. The pixels are stored, not compressed (a
// PNG's deflate stream may hold stored blocks, which need only two checksums), so the file is the picture's size and
// nothing here depends on a compression library. Any PNG reader opens it.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tee::files
{
    // The file's bytes for w x h pixels at `rgba` (w * h * 4 bytes). Empty when w or h is not above 0.
    std::vector<std::uint8_t> encodePng(const std::uint8_t* rgba, int w, int h);

    // Writes it. False, and why, when the file cannot be written.
    bool writePng(const std::string& path, const std::uint8_t* rgba, int w, int h, std::string& why);
}
