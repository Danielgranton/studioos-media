#pragma once

#include <string>

#include "core/Result.hpp"
#include "media/VideoProcessor.hpp"

class VideoService
{
public:
    Result<std::string> compress(const std::string& path);

    Result<std::string> thumbnail(const std::string& path);

    Result<std::string> extractAudio(const std::string& path);

    Result<std::string> convert(const std::string& path, const std::string& format);

    Result<std::string> trim(const std::string& path, const std::string& start, const std::string& end);

    Result<std::string> merge(const std::vector<std::string>& paths, const std::string& outputPath);

private:
    VideoProcessor processor;
};