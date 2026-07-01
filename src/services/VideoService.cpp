#include "VideoService.hpp"

Result<std::string> VideoService::compress(const std::string& path)
{
    return processor.compress(path);
}

Result<std::string> VideoService::thumbnail(const std::string& path)
{
    return processor.thumbnail(path);
}

Result<std::string> VideoService::extractAudio(const std::string& path)
{
    return processor.extractAudio(path);
}