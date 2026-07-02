#include "AudioProcessor.hpp"

#include <fstream>

#include "config/config.hpp"
#include "ffmpeg/FFmpeg.hpp"
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

Result<std::string> AudioProcessor::encode(const std::string& inputPath, const std::string& format)
{
    Timer timer;
    Logger::info("Encoding audio: " + inputPath + " -> " + format);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Audio input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::AUDIO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "encoded_", "." + format);
    const std::string args = "-y -i \"" + inputPath + "\" -c:a " + format + " \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to encode audio with FFmpeg", StatusCode::AUDIO_ERROR);
    }

    Logger::info("Audio encode completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> AudioProcessor::normalize(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Normalizing audio: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Audio input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::AUDIO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "normalized_", ".mp3");
    const std::string args = "-y -i \"" + inputPath + "\" -af loudnorm=I=-16:TP=-1.5:LRA=11 \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to normalize audio with FFmpeg", StatusCode::AUDIO_ERROR);
    }

    Logger::info("Audio normalize completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> AudioProcessor::denoise(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Reducing noise in audio: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Audio input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::AUDIO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "denoised_", ".mp3");
    const std::string args = "-y -i \"" + inputPath + "\" -af afftdn \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to denoise audio with FFmpeg", StatusCode::AUDIO_ERROR);
    }

    Logger::info("Audio denoise completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> AudioProcessor::merge(const std::vector<std::string>& inputPaths, const std::string& outputPath)
{
    Timer timer;
    Logger::info("Merging audio inputs");

    if (inputPaths.empty())
    {
        return Result<std::string>::fail("No input audio files provided", StatusCode::AUDIO_ERROR);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::AUDIO_ERROR);
    }

    for (const auto& inputPath : inputPaths)
    {
        if (inputPath.empty() || !FileUtils::exists(inputPath))
        {
            return Result<std::string>::fail("Audio input not found", StatusCode::FILE_NOT_FOUND);
        }
    }

    const std::string outputDirectory = FileUtils::filename(outputPath).empty() ? "" : outputPath.substr(0, outputPath.find_last_of('/'));
    if (!outputDirectory.empty())
    {
        FileUtils::createDirectory(outputDirectory);
    }

    std::string args = "-y";
    for (const auto& inputPath : inputPaths)
    {
        args += " -i \"" + inputPath + "\"";
    }

    std::string filter = " -filter_complex \"";
    filter += "[0:a]";
    for (std::size_t index = 1; index < inputPaths.size(); ++index)
    {
        filter += "[" + std::to_string(index) + ":a]";
    }
    filter += "concat=n=" + std::to_string(inputPaths.size()) + ":v=0:a=1[aout]\" -map \"[aout]\"";

    args += filter;

    const std::string extension = FileUtils::extension(outputPath);
    if (extension == ".mp3")
    {
        args += " -c:a libmp3lame";
    }
    else if (extension == ".aac" || extension == ".m4a")
    {
        args += " -c:a aac";
    }
    else if (extension == ".wav")
    {
        args += " -c:a pcm_s16le";
    }
    else if (extension == ".ogg")
    {
        args += " -c:a libvorbis";
    }

    args += " \"" + outputPath + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to merge audio with FFmpeg", StatusCode::AUDIO_ERROR);
    }

    Logger::info("Audio merge completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(outputPath);
}

Result<std::string> AudioProcessor::trim(const std::string& inputPath, const std::string& start, const std::string& end)
{
    Timer timer;
    Logger::info("Trimming audio: " + inputPath);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Audio input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::AUDIO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "trimmed_audio_", ".mp3");
    const std::string args = "-y -i \"" + inputPath + "\" -ss " + start + " -to " + end + " \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to trim audio with FFmpeg", StatusCode::AUDIO_ERROR);
    }

    Logger::info("Audio trim completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> AudioProcessor::convert(const std::string& inputPath, const std::string& format)
{
    Timer timer;
    Logger::info("Converting audio: " + inputPath + " -> " + format);

    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Audio input not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::AUDIO_ERROR);
    }

    const std::string output = makeOutputPath(inputPath, "converted_audio_", "." + format);
    const std::string args = "-y -i \"" + inputPath + "\" -c:a " + format + " \"" + output + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to convert audio with FFmpeg", StatusCode::AUDIO_ERROR);
    }

    Logger::info("Audio convert completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}
