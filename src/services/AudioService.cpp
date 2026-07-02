#include "AudioService.hpp"

Result<std::string> AudioService::encode(const std::string& path, const std::string& format)
{
    return processor.encode(path, format);
}

Result<std::string> AudioService::normalize(const std::string& path)
{
    return processor.normalize(path);
}

Result<std::string> AudioService::denoise(const std::string& path)
{
    return processor.denoise(path);
}

Result<std::string> AudioService::merge(const std::vector<std::string>& paths, const std::string& outputPath)
{
    return processor.merge(paths, outputPath);
}

Result<std::string> AudioService::trim(const std::string& path, const std::string& start, const std::string& end)
{
    return processor.trim(path, start, end);
}

Result<std::string> AudioService::convert(const std::string& path, const std::string& format)
{
    return processor.convert(path, format);
}
