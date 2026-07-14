#include "VideoService.hpp"

#include <nlohmann/json.hpp>

Result<std::string> VideoService::compress(const std::string& path)
{
    return processor.compress(path);
}

Result<std::string> VideoService::resize(const std::string& path, int width, int height)
{
    return processor.resize(path, width, height);
}

Result<std::string> VideoService::watermark(const std::string& path, const std::string& watermarkPath, int x, int y)
{
    return processor.watermark(path, watermarkPath, x, y);
}

Result<std::string> VideoService::rotate(const std::string& path, int degrees)
{
    return processor.rotate(path, degrees);
}

Result<std::string> VideoService::normalizeAudio(const std::string& path)
{
    return processor.normalizeAudio(path);
}

Result<std::string> VideoService::thumbnails(const std::string& path, int count)
{
    return processor.thumbnails(path, count);
}

Result<std::string> VideoService::gifPreview(const std::string& path, int durationSeconds)
{
    return processor.gifPreview(path, durationSeconds);
}

Result<std::string> VideoService::previewClip(const std::string& path, int durationSeconds)
{
    return processor.previewClip(path, durationSeconds);
}

Result<std::string> VideoService::adaptiveStreaming(const std::string& path, const std::string& outputDirectory)
{
    return processor.adaptiveStreaming(path, outputDirectory);
}

Result<std::string> VideoService::metadata(const std::string& path)
{
    const auto result = processor.metadata(path);
    if (!result.success)
    {
        return Result<std::string>::fail(result.message, result.status);
    }

    nlohmann::json payload = {
        {"width", result.value.width},
        {"height", result.value.height},
        {"codec", result.value.codec},
        {"fps", result.value.fps},
        {"durationSeconds", result.value.durationSeconds},
        {"bitrate", result.value.bitrate},
        {"sizeBytes", result.value.sizeBytes},
        {"audioCodec", result.value.audioCodec},
        {"audioChannels", result.value.audioChannels},
        {"audioSampleRate", result.value.audioSampleRate}
    };

    return Result<std::string>::ok(payload.dump());
}

Result<std::string> VideoService::extractFrame(const std::string& path, const std::string& timestamp)
{
    return processor.extractFrame(path, timestamp);
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
