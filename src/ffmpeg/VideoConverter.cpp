#include "VideoConverter.hpp"

#include <string>

#include "FFmpeg.hpp"

bool VideoConverter::convert(const std::string& inputPath, const std::string& outputPath, const std::string& format)
{
    const std::string args = "-y -i \"" + inputPath + "\" -c:v libx264 -c:a aac -f " + format + " \"" + outputPath + "\"";
    return FFmpeg::run(args);
}
