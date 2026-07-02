#pragma once

#include <string>

class ThumbnailGenerator
{
public:
    static bool generate(const std::string& inputPath, const std::string& outputPath);
};
