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

Result<std::string> VideoService::convert(const std::string& path, const std::string& format)
{
    return processor.convert(path, format);
}

Result<std::string> VideoService::trim(const std::string& path, const std::string& start, const std::string& end)
{
    return processor.trim(path, start, end);
}

Result<std::string> VideoService::merge(const std::vector<std::string>& paths, const std::string& outputPath)
{
    return processor.merge(paths, outputPath);
}