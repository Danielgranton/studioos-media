#include "AudioService.hpp"

Result<std::string> AudioService::encode(const std::string& path, const std::string& format)
{
    return processor.encode(path, format);
}

Result<std::string> AudioService::normalize(
    const std::string& path, int durationSeconds, const std::function<void(int)>& onProgress)
{
    return processor.normalize(path, durationSeconds, onProgress);
}

Result<std::string> AudioService::denoise(const std::string& path)
{
    return processor.denoise(path);
}

Result<std::string> AudioService::merge(const std::vector<std::string>& paths, const std::string& outputPath)
{
    return processor.merge(paths, outputPath);
}

Result<std::string> AudioService::trim(
    const std::string& path, const std::string& start, const std::string& end,
    int durationSeconds, const std::function<void(int)>& onProgress)
{
    return processor.trim(path, start, end, durationSeconds, onProgress);
}

Result<std::string> AudioService::waveform(const std::string& path)
{
    return processor.waveform(path);
}

Result<std::string> AudioService::convert(const std::string& path, const std::string& format)
{
    return processor.convert(path, format);
}
