#include "AudioEncoder.hpp"

#include <string>

#include "FFmpeg.hpp"

bool AudioEncoder::extract(const std::string& inputPath, const std::string& outputPath)
{
    const std::string args = "-y -i \"" + inputPath + "\" -vn -acodec libmp3lame -q:a 2 \"" + outputPath + "\"";
    return FFmpeg::run(args);
}
