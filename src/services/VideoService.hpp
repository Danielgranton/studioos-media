#pragma once

#include <vector>
#include <string>

#include "core/Result.hpp"
#include "media/VideoProcessor.hpp"

class VideoService
{
public:
    Result<std::string> compress(const std::string& path);

    Result<std::string> resize(const std::string& path, int width, int height);

    Result<std::string> watermark(const std::string& path, const std::string& watermarkPath, int x, int y);

    Result<std::string> rotate(const std::string& path, int degrees);

    Result<std::string> normalizeAudio(const std::string& path);

    Result<std::string> thumbnails(const std::string& path, int count = 4);

    Result<std::string> gifPreview(const std::string& path, int durationSeconds = 5);

    Result<std::string> previewClip(const std::string& path, int durationSeconds = 10);

    Result<std::string> adaptiveStreaming(const std::string& path, const std::string& outputDirectory);

    Result<std::string> metadata(const std::string& path);

    Result<std::string> extractFrame(const std::string& path, const std::string& timestamp);

    Result<std::string> thumbnail(const std::string& path);

    Result<std::string> extractAudio(const std::string& path);

    Result<std::string> convert(const std::string& path, const std::string& format);

    Result<std::string> trim(const std::string& path, const std::string& start, const std::string& end);

    Result<std::string> merge(const std::vector<std::string>& paths, const std::string& outputPath);

private:
    VideoProcessor processor;
};
