#include "VideoProcessor.hpp"

#include <fstream>

#include "config/config.hpp"
#include "ffmpeg/AudioEncoder.hpp"
#include "ffmpeg/FFmpeg.hpp"
#include "ffmpeg/ThumbnailGenerator.hpp"
#include "ffmpeg/VideoConverter.hpp"
#include "ffmpeg/VideoEncoder.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"
#include "utils/Timer.hpp"

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
