#include "VideoService.hpp"

std::string VideoService::compress(const std::string& path)
{
    return "compressed_" + path;
}

std::string VideoService::thumbnail(const std::string& path)
{
    return "thumbnail_" + path;
}

std::string VideoService::extractAudio(const std::string& path)
{
    return "audio_" + path;
}