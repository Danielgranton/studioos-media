#include "ImageProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

std::string ImageProcessor::compress(
    const std::string& inputPath,
    int quality)
{
    cv::Mat image = cv::imread(inputPath);

    if (image.empty())
    {
        return "";
    }

    fs::path path(inputPath);

    std::string output =
        "temp/compressed_" +
        path.filename().string();

    std::vector<int> params =
    {
        cv::IMWRITE_JPEG_QUALITY,
        quality
    };

    cv::imwrite(output, image, params);

    return output;
}

std::string ImageProcessor::resize(
    const std::string& inputPath,
    int width,
    int height)
{
    cv::Mat image = cv::imread(inputPath);

    if (image.empty())
    {
        return "";
    }

    cv::Mat resized;

    cv::resize(
        image,
        resized,
        cv::Size(width, height)
    );

    fs::path path(inputPath);

    std::string output =
        "temp/resized_" +
        path.filename().string();

    cv::imwrite(output, resized);

    return output;
}

std::string ImageProcessor::thumbnail(
    const std::string& inputPath,
    int size)
{
    return resize(
        inputPath,
        size,
        size
    );
}