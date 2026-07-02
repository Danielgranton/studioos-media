#pragma once

#include <string>
#include <vector>

#include "core/Result.hpp"
#include "media/AudioProcessor.hpp"

class AudioService
{
public:
    Result<std::string> encode(const std::string& path, const std::string& format);

    Result<std::string> normalize(const std::string& path);

    Result<std::string> denoise(const std::string& path);

    Result<std::string> merge(const std::vector<std::string>& paths, const std::string& outputPath);

    Result<std::string> trim(const std::string& path, const std::string& start, const std::string& end);

    Result<std::string> convert(const std::string& path, const std::string& format);

private:
    AudioProcessor processor;
};
