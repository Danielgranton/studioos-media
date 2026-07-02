#pragma once

#include <string>
#include <vector>

#include "core/Result.hpp"

class AudioProcessor
{
public:
    Result<std::string> encode(const std::string& inputPath, const std::string& format);

    Result<std::string> normalize(const std::string& inputPath);

    Result<std::string> denoise(const std::string& inputPath);

    Result<std::string> merge(const std::vector<std::string>& inputPaths, const std::string& outputPath);

    Result<std::string> trim(const std::string& inputPath, const std::string& start, const std::string& end);

    Result<std::string> convert(const std::string& inputPath, const std::string& format);
};
