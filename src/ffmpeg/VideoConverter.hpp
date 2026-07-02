#pragma once

#include <string>

class VideoConverter
{
public:
    static bool convert(const std::string& inputPath, const std::string& outputPath, const std::string& format);
};
