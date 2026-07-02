#include "ThumbnailGenerator.hpp"

#include <string>

#include "FFmpeg.hpp"

bool ThumbnailGenerator::generate(const std::string& inputPath, const std::string& outputPath)
{
    const std::string args = "-y -i \"" + inputPath + "\" -ss 00:00:00.2 -frames:v 1 -update 1 -q:v 2 -f image2 \"" + outputPath + "\"";
    return FFmpeg::run(args);
}
