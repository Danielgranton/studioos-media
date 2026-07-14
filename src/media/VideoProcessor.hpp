#pragma once

#include <string>
#include <vector>

#include "core/Result.hpp"

struct VideoMetadata
{
    int width = 0;
    int height = 0;
    std::string codec;
    double fps = 0.0;
    double durationSeconds = 0.0;
    long long bitrate = 0;
    std::size_t sizeBytes = 0;
    std::string audioCodec;
    int audioChannels = 0;
    int audioSampleRate = 0;
};

class VideoProcessor
{
public:
    Result<std::string> compress(const std::string& inputPath);

    Result<std::string> resize(const std::string& inputPath, int width, int height);

    Result<std::string> watermark(
        const std::string& inputPath,
        const std::string& watermarkPath,
        int x,
        int y);

    Result<std::string> rotate(const std::string& inputPath, int degrees);

    Result<std::string> normalizeAudio(const std::string& inputPath);

    Result<std::string> thumbnails(const std::string& inputPath, int count = 4);

    Result<std::string> gifPreview(const std::string& inputPath, int durationSeconds = 5);

    Result<std::string> previewClip(const std::string& inputPath, int durationSeconds = 10);

    Result<std::string> adaptiveStreaming(const std::string& inputPath, const std::string& outputDirectory);

    Result<VideoMetadata> metadata(const std::string& inputPath);

    Result<std::string> extractFrame(const std::string& inputPath, const std::string& timestamp);

    Result<std::string> thumbnail(const std::string& inputPath);

    Result<std::string> extractAudio(const std::string& inputPath);

    Result<std::string> convert(const std::string& inputPath, const std::string& format);

    Result<std::string> trim(const std::string& inputPath, const std::string& start, const std::string& end);

    Result<std::string> merge(const std::vector<std::string>& inputPaths, const std::string& outputPath);
};
