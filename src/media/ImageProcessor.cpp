#include "ImageProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <vector>

#include "config/config.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"
#include "utils/Timer.hpp"

Result<std::string> ImageProcessor::compress(
    const std::string& inputPath,
    int quality)
{
    Timer timer;
    Logger::info("Compressing image: " + inputPath);

    cv::Mat image = cv::imread(inputPath);

    if (image.empty())
    {
        return Result<std::string>::fail(
            "Unable to load image",
            StatusCode::FILE_NOT_FOUND
        );
    }

    const std::string output =
        Config::instance().tempFolder() + "/compressed_" +
        FileUtils::filename(inputPath);

    std::vector<int> params =
    {
        cv::IMWRITE_JPEG_QUALITY,
        quality
    };

    if (!cv::imwrite(output, image, params))
    {
        Logger::error("Image compression failed for " + inputPath);
        return Result<std::string>::fail(
            "Failed to write compressed image"
        );
    }

    Logger::info("Compression completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::resize(
    const std::string& inputPath,
    int width,
    int height)
{
    Timer timer;
    Logger::info("Resizing image: " + inputPath);

    cv::Mat image = cv::imread(inputPath);

    if (image.empty())
    {
        return Result<std::string>::fail(
            "Unable to load image",
            StatusCode::FILE_NOT_FOUND
        );
    }

    cv::Mat resized;

    cv::resize(
        image,
        resized,
        cv::Size(width, height)
    );

    const std::string output =
        Config::instance().tempFolder() + "/resized_" +
        FileUtils::filename(inputPath);

    if (!cv::imwrite(output, resized))
    {
        Logger::error("Image resize failed for " + inputPath);
        return Result<std::string>::fail(
            "Failed to save resized image"
        );
    }

    Logger::info("Resize completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::thumbnail(
    const std::string& inputPath,
    int size)
{
    return resize(
        inputPath,
        size,
        size
    );
}