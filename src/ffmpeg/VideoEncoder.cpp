#include "VideoEncoder.hpp"

#include <string>

#include "FFmpeg.hpp"

bool VideoEncoder::compress(const std::string& inputPath, const std::string& outputPath)
{
    const std::string args = "-y -i \"" + inputPath + "\" -c:v libx264 -preset fast -crf 23 -c:a aac -b:a 128k \"" + outputPath + "\"";
    return FFmpeg::run(args);
}
