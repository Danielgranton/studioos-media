#pragma once

#include <string>

class VideoEncoder
{
public:
    static bool compress(const std::string& inputPath, const std::string& outputPath);
};
