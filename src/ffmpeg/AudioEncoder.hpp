#pragma once

#include <string>

class AudioEncoder
{
public:
    static bool extract(const std::string& inputPath, const std::string& outputPath);
};
