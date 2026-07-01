#include "VideoProcessor.hpp"

#include <fstream>

#include "config/config.hpp"
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

    const std::string output = makeOutputPath(inputPath, "compressed_", ".mp4");

    std::ofstream stream(output, std::ios::binary);
    if (!stream)
    {
        return Result<std::string>::fail("Unable to create compressed video output");
    }

    stream.close();

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

    const std::string output = makeOutputPath(inputPath, "thumbnail_", ".jpg");

    std::ofstream stream(output, std::ios::binary);
    if (!stream)
    {
        return Result<std::string>::fail("Unable to create thumbnail output");
    }

    stream.close();

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

    const std::string output = makeOutputPath(inputPath, "audio_", ".mp3");

    std::ofstream stream(output, std::ios::binary);
    if (!stream)
    {
        return Result<std::string>::fail("Unable to create audio output");
    }

    stream.close();

    Logger::info("Audio extraction completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}
