#include "VideoProcessor.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>

#include "config/config.hpp"
#include "ffmpeg/AudioEncoder.hpp"
#include "ffmpeg/CommandRunner.hpp"
#include "ffmpeg/FFmpeg.hpp"
#include "ffmpeg/ThumbnailGenerator.hpp"
#include "ffmpeg/VideoConverter.hpp"
#include "ffmpeg/VideoEncoder.hpp"
#include "nlohmann/json.hpp"
#include "services/StreamingService.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"
#include "utils/Timer.hpp"

using json = nlohmann::json;

namespace
{
std::string makeOutputPath(
    const std::string& inputPath,
    const std::string& prefix,
    const std::string& extension)
{
    const std::string tempFolder = Config::instance().tempFolder();
    FileUtils::createDirectory(tempFolder);

    return tempFolder + "/" + prefix + FileUtils::stem(inputPath) + extension;
}

std::string makeOutputDirectory(
    const std::string& inputPath,
    const std::string& prefix)
{
    const std::string tempFolder = Config::instance().tempFolder();
    FileUtils::createDirectory(tempFolder);
    return tempFolder + "/" + prefix + FileUtils::stem(inputPath) + "_" + std::to_string(std::rand() % 1000000);
}

bool parseFraction(const std::string& value, double* output)
{
    if (output == nullptr || value.empty())
    {
        return false;
    }

    const auto slash = value.find('/');
    try
    {
        if (slash == std::string::npos)
        {
            *output = std::stod(value);
            return true;
        }

        const double numerator = std::stod(value.substr(0, slash));
        const double denominator = std::stod(value.substr(slash + 1));
        if (denominator == 0.0)
        {
            return false;
        }

        *output = numerator / denominator;
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

double readDouble(const json& node, const char* key, double fallback)
{
    if (!node.contains(key) || node[key].is_null())
    {
        return fallback;
    }

    try
    {
        if (node[key].is_number())
        {
            return node[key].get<double>();
        }

        if (node[key].is_string())
        {
            return std::stod(node[key].get<std::string>());
        }
    }
    catch (const std::exception&)
    {
    }

    return fallback;
}

long long readLongLong(const json& node, const char* key, long long fallback)
{
    if (!node.contains(key) || node[key].is_null())
    {
        return fallback;
    }

    try
    {
        if (node[key].is_number_integer() || node[key].is_number_unsigned())
        {
            return node[key].get<long long>();
        }

        if (node[key].is_string())
        {
            return std::stoll(node[key].get<std::string>());
        }
    }
    catch (const std::exception&)
    {
    }

    return fallback;
}

int readInt(const json& node, const char* key, int fallback)
{
    const auto value = readLongLong(node, key, fallback);
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
    {
        return fallback;
    }

    return static_cast<int>(value);
}
}

Result<std::string> VideoProcessor::compress(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Compressing video: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "compressed_", ".mp4");

    if (!VideoEncoder::compress(inputPath, output))
    {
        return Result<std::string>::fail("Failed to compress video with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Video compression completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::resize(const std::string& inputPath, int width, int height)
{
    Timer timer;
    Logger::info("Resizing video: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (width <= 0 || height <= 0)
    {
        return Result<std::string>::fail("Invalid resize dimensions", StatusCode::INVALID_FORMAT);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "resized_", ".mp4");
    const std::string filter =
        "scale=w=" + std::to_string(width) + ":h=" + std::to_string(height) +
        ":force_original_aspect_ratio=decrease,pad=" + std::to_string(width) + ":" +
        std::to_string(height) + ":(ow-iw)/2:(oh-ih)/2";
    const std::string args =
        "-y -i \"" + inputPath + "\" -vf \"" + filter +
        "\" -c:v libx264 -preset fast -crf 23 -c:a copy \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to resize video with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Video resize completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::watermark(
    const std::string& inputPath,
    const std::string& watermarkPath,
    int x,
    int y)
{
    Timer timer;
    Logger::info("Watermarking video: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (watermarkPath.empty() || !FileUtils::exists(watermarkPath))
    {
        return Result<std::string>::fail("Watermark image not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "watermarked_", ".mp4");
    const std::string args =
        "-y -i \"" + inputPath + "\" -i \"" + watermarkPath +
        "\" -filter_complex \"[1:v]format=rgba[wm];[0:v][wm]overlay=" +
        std::to_string(x) + ":" + std::to_string(y) +
        ":format=auto\" -c:v libx264 -preset fast -crf 23 -c:a copy \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to watermark video with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Video watermark completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::rotate(const std::string& inputPath, int degrees)
{
    Timer timer;
    Logger::info("Rotating video: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const int normalized = ((degrees % 360) + 360) % 360;
    std::string filter;
    switch (normalized)
    {
    case 0:
        filter = "null";
        break;
    case 90:
        filter = "transpose=1";
        break;
    case 180:
        filter = "hflip,vflip";
        break;
    case 270:
        filter = "transpose=2";
        break;
    default:
        filter = "rotate=" + std::to_string(static_cast<double>(normalized) * 3.14159265358979323846 / 180.0);
        break;
    }

    const std::string output = makeOutputPath(inputPath, "rotated_", ".mp4");
    const std::string args =
        "-y -i \"" + inputPath + "\" -vf \"" + filter +
        "\" -c:v libx264 -preset fast -crf 23 -c:a copy \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to rotate video with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Video rotate completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::normalizeAudio(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Normalizing video audio: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "audio_normalized_", ".mp4");
    const std::string args =
        "-y -i \"" + inputPath +
        "\" -c:v copy -af loudnorm=I=-16:TP=-1.5:LRA=11 -c:a aac -movflags +faststart \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to normalize video audio with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Video audio normalization completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::thumbnails(const std::string& inputPath, int count)
{
    Timer timer;
    Logger::info("Generating video thumbnails: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (count <= 0)
    {
        return Result<std::string>::fail("Invalid thumbnail count", StatusCode::INVALID_FORMAT);
    }

    const auto metadataResult = metadata(inputPath);
    if (!metadataResult.success)
    {
        return Result<std::string>::fail(metadataResult.message, metadataResult.status);
    }

    const std::string outputDirectory = makeOutputDirectory(inputPath, "thumbnails_");
    FileUtils::createDirectory(outputDirectory);

    const double duration = std::max(1.0, metadataResult.value.durationSeconds);
    for (int index = 0; index < count; ++index)
    {
        const double position = std::min(duration - 0.1, duration * static_cast<double>(index + 1) / static_cast<double>(count + 1));
        const std::string thumbnailPath = outputDirectory + "/thumbnail_" + std::to_string(index + 1) + ".jpg";
        const std::string args =
            "-y -ss " + std::to_string(position) + " -i \"" + inputPath +
            "\" -frames:v 1 -q:v 2 -f image2 \"" + thumbnailPath + "\"";
        if (!FFmpeg::run(args))
        {
            return Result<std::string>::fail("Failed to generate one or more thumbnails", StatusCode::VIDEO_ERROR);
        }
    }

    Logger::info("Video thumbnails completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(outputDirectory);
}

Result<std::string> VideoProcessor::gifPreview(const std::string& inputPath, int durationSeconds)
{
    Timer timer;
    Logger::info("Generating GIF preview: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (durationSeconds <= 0)
    {
        return Result<std::string>::fail("Invalid preview duration", StatusCode::INVALID_FORMAT);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "preview_", ".gif");
    const std::string args =
        "-y -ss 0 -t " + std::to_string(durationSeconds) + " -i \"" + inputPath +
        "\" -vf \"fps=12,scale=640:-1:flags=lanczos\" -loop 0 \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to generate GIF preview", StatusCode::VIDEO_ERROR);
    }

    Logger::info("GIF preview completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::previewClip(const std::string& inputPath, int durationSeconds)
{
    Timer timer;
    Logger::info("Generating preview clip: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (durationSeconds <= 0)
    {
        return Result<std::string>::fail("Invalid preview duration", StatusCode::INVALID_FORMAT);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "preview_clip_", ".mp4");
    const std::string args =
        "-y -ss 0 -t " + std::to_string(durationSeconds) + " -i \"" + inputPath +
        "\" -c:v libx264 -preset fast -crf 23 -c:a aac -movflags +faststart \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to generate preview clip", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Preview clip completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::adaptiveStreaming(const std::string& inputPath, const std::string& outputDirectory)
{
    StreamingService service;
    return service.generateAdaptiveBitrate(inputPath, outputDirectory);
}

Result<VideoMetadata> VideoProcessor::metadata(const std::string& inputPath)
{
    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<VideoMetadata>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<VideoMetadata>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    std::string output;
    const std::string command = "ffprobe -v error -show_streams -show_format -of json \"" + inputPath + "\"";
    if (!CommandRunner::run(command, &output))
    {
        return Result<VideoMetadata>::fail("Failed to read video metadata", StatusCode::VIDEO_ERROR);
    }

    try
    {
        const json payload = json::parse(output);
        VideoMetadata metadata;
        metadata.sizeBytes = FileUtils::fileSize(inputPath);

        if (payload.contains("format"))
        {
            const auto& format = payload["format"];
            metadata.durationSeconds = readDouble(format, "duration", 0.0);
            metadata.bitrate = readLongLong(format, "bit_rate", 0LL);
            metadata.sizeBytes = static_cast<std::size_t>(readLongLong(format, "size", static_cast<long long>(metadata.sizeBytes)));
        }

        if (payload.contains("streams") && payload["streams"].is_array())
        {
            for (const auto& stream : payload["streams"])
            {
                const std::string type = stream.value("codec_type", "");
                if (type == "video" && metadata.codec.empty())
                {
                    metadata.codec = stream.value("codec_name", "");
                    metadata.width = readInt(stream, "width", 0);
                    metadata.height = readInt(stream, "height", 0);
                    const std::string fpsValue = stream.value("r_frame_rate", "0/1");
                    parseFraction(fpsValue, &metadata.fps);
                }
                else if (type == "audio" && metadata.audioCodec.empty())
                {
                    metadata.audioCodec = stream.value("codec_name", "");
                    metadata.audioChannels = readInt(stream, "channels", 0);
                    metadata.audioSampleRate = readInt(stream, "sample_rate", 0);
                }
            }
        }

        return Result<VideoMetadata>::ok(metadata);
    }
    catch (const std::exception&)
    {
        return Result<VideoMetadata>::fail("Failed to parse video metadata", StatusCode::VIDEO_ERROR);
    }
}

Result<std::string> VideoProcessor::extractFrame(const std::string& inputPath, const std::string& timestamp)
{
    Timer timer;
    Logger::info("Extracting video frame: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (timestamp.empty())
    {
        return Result<std::string>::fail("Frame timestamp is required", StatusCode::INVALID_FORMAT);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "frame_", ".jpg");
    const std::string args =
        "-y -ss " + timestamp + " -i \"" + inputPath +
        "\" -frames:v 1 -q:v 2 -f image2 \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to extract video frame", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Frame extraction completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::thumbnail(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Generating thumbnail: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "thumbnail_", ".jpg");

    if (!ThumbnailGenerator::generate(inputPath, output))
    {
        return Result<std::string>::fail("Failed to generate thumbnail with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Thumbnail generation completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::extractAudio(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Extracting audio: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "audio_", ".mp3");

    if (!AudioEncoder::extract(inputPath, output))
    {
        return Result<std::string>::fail("Failed to extract audio with FFmpeg", StatusCode::AUDIO_ERROR);
    }

    Logger::info("Audio extraction completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::convert(const std::string& inputPath, const std::string& format)
{
    Timer timer;
    Logger::info("Converting video: " + inputPath + " to " + format);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "converted_", "." + format);

    if (!VideoConverter::convert(inputPath, output, format))
    {
        return Result<std::string>::fail("Failed to convert video with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Video conversion completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::trim(const std::string& inputPath, const std::string& start, const std::string& end)
{
    Timer timer;
    Logger::info("Trimming video: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "trimmed_", ".mp4");

    const std::string args = "-y -i \"" + inputPath + "\" -ss " + start + " -to " + end + " -c:v libx264 -c:a aac \"" + output + "\"";
    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to trim video with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    Logger::info("Video trim completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> VideoProcessor::merge(const std::vector<std::string>& inputPaths, const std::string& outputPath)
{
    Timer timer;
    Logger::info("Merging videos");

    if (inputPaths.empty())
    {
        return Result<std::string>::fail("No input videos provided", StatusCode::VIDEO_ERROR);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    std::string listFile = FileUtils::tempFile("ffmpeg-list-");
    std::ofstream listStream(listFile, std::ios::out);
    if (!listStream)
    {
        return Result<std::string>::fail("Failed to create merge list", StatusCode::INTERNAL_ERROR);
    }

    for (const auto& inputPath : inputPaths)
    {
        if (inputPath.empty() || !FileUtils::exists(inputPath))
        {
            return Result<std::string>::fail("Video input not found", StatusCode::FILE_NOT_FOUND);
        }
        listStream << "file '" << inputPath << "'\n";
    }
    listStream.close();

    const std::string args = "-y -f concat -safe 0 -i \"" + listFile + "\" -c copy \"" + outputPath + "\"";
    if (!FFmpeg::run(args))
    {
        FileUtils::deleteFile(listFile);
        return Result<std::string>::fail("Failed to merge videos with FFmpeg", StatusCode::VIDEO_ERROR);
    }

    FileUtils::deleteFile(listFile);
    Logger::info("Video merge completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(outputPath);
}
