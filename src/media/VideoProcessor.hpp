#pragma once

#include <string>

#include "core/Result.hpp"

class VideoProcessor
{
public:
    Result<std::string> compress(const std::string& inputPath);

    Result<std::string> thumbnail(const std::string& inputPath);

    Result<std::string> extractAudio(const std::string& inputPath);
};
