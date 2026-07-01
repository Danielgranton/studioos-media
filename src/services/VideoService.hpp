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

private:
    VideoProcessor processor;
};