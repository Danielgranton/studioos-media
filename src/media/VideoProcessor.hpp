#pragma once

#include <string>
#include <vector>

#include "core/Result.hpp"

class VideoProcessor
{
public:
    Result<std::string> compress(const std::string& inputPath);

    Result<std::string> thumbnail(const std::string& inputPath);

    Result<std::string> extractAudio(const std::string& inputPath);

    Result<std::string> convert(const std::string& inputPath, const std::string& format);

    Result<std::string> trim(const std::string& inputPath, const std::string& start, const std::string& end);

    Result<std::string> merge(const std::vector<std::string>& inputPaths, const std::string& outputPath);
};
