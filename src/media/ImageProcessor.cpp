#include "ImageProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <vector>

#include "config/config.hpp"
#include "ffmpeg/FFmpeg.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"
#include "utils/Timer.hpp"

namespace
{
cv::Mat loadImage(const std::string& inputPath)
{
    return cv::imread(inputPath);
}

std::string outputPath(const std::string& inputPath, const std::string& prefix)
{
    return Config::instance().tempFolder() + "/" + prefix + FileUtils::filename(inputPath);
}

bool writeImage(const std::string& output, const cv::Mat& image, const std::vector<int>& params = {})
{
    try
    {
        return cv::imwrite(output, image, params);
    }
    catch (const cv::Exception&)
    {
        return false;
    }
}
}

Result<std::string> ImageProcessor::compress(const std::string& inputPath, int quality)
{
    Timer timer;
    Logger::info("Compressing image: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    const std::string output = outputPath(inputPath, "compressed_");
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};

    if (!writeImage(output, image, params))
    {
        Logger::error("Image compression failed for " + inputPath);
        return Result<std::string>::fail("Failed to write compressed image");
    }

    Logger::info("Compression completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::resize(const std::string& inputPath, int width, int height)
{
    Timer timer;
    Logger::info("Resizing image: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(width, height));

    const std::string output = outputPath(inputPath, "resized_");
    if (!writeImage(output, resized))
    {
        Logger::error("Image resize failed for " + inputPath);
        return Result<std::string>::fail("Failed to save resized image");
    }

    Logger::info("Resize completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::thumbnail(const std::string& inputPath, int size)
{
    return resize(inputPath, size, size);
}

Result<std::string> ImageProcessor::crop(const std::string& inputPath, int x, int y, int width, int height)
{
    Timer timer;
    Logger::info("Cropping image: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    cv::Rect region(x, y, width, height);
    cv::Mat cropped = image(region).clone();
    const std::string output = outputPath(inputPath, "cropped_");

    if (!writeImage(output, cropped))
    {
        return Result<std::string>::fail("Failed to crop image");
    }

    Logger::info("Crop completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::rotate(const std::string& inputPath, double angle)
{
    Timer timer;
    Logger::info("Rotating image: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    cv::Point2f center(static_cast<float>(image.cols / 2), static_cast<float>(image.rows / 2));
    cv::Mat rotation = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::Mat rotated;
    cv::warpAffine(image, rotated, rotation, image.size());

    const std::string output = outputPath(inputPath, "rotated_");
    if (!writeImage(output, rotated))
    {
        return Result<std::string>::fail("Failed to rotate image");
    }

    Logger::info("Rotate completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::watermark(const std::string& inputPath, const std::string& watermarkPath, int x, int y)
{
    Timer timer;
    Logger::info("Applying watermark: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    cv::Mat watermark = loadImage(watermarkPath);
    if (image.empty() || watermark.empty())
    {
        return Result<std::string>::fail("Unable to load image or watermark", StatusCode::FILE_NOT_FOUND);
    }

    cv::Rect roi(x, y, watermark.cols, watermark.rows);
    if (roi.x + roi.width > image.cols || roi.y + roi.height > image.rows)
    {
        return Result<std::string>::fail("Watermark region is out of bounds", StatusCode::INVALID_FORMAT);
    }

    cv::Mat overlay;
    watermark.copyTo(image(roi));

    const std::string output = outputPath(inputPath, "watermarked_");
    if (!writeImage(output, image))
    {
        return Result<std::string>::fail("Failed to write watermarked image");
    }

    Logger::info("Watermark completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::blur(const std::string& inputPath, int kernelSize)
{
    Timer timer;
    Logger::info("Blurring image: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    cv::Mat blurred;
    cv::GaussianBlur(image, blurred, cv::Size(kernelSize, kernelSize), 0);

    const std::string output = outputPath(inputPath, "blurred_");
    if (!writeImage(output, blurred))
    {
        return Result<std::string>::fail("Failed to blur image");
    }

    Logger::info("Blur completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::sharpen(const std::string& inputPath, double amount)
{
    Timer timer;
    Logger::info("Sharpening image: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    cv::Mat sharpened;
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -amount, 0, -amount, 1 + 4 * amount, -amount, 0, -amount, 0);
    cv::filter2D(image, sharpened, -1, kernel);

    const std::string output = outputPath(inputPath, "sharpened_");
    if (!writeImage(output, sharpened))
    {
        return Result<std::string>::fail("Failed to sharpen image");
    }

    Logger::info("Sharpen completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::removeMetadata(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Removing metadata: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    const std::string output = outputPath(inputPath, "metadata_removed_");
    if (!writeImage(output, image))
    {
        return Result<std::string>::fail("Failed to remove metadata");
    }

    Logger::info("Metadata removal completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::saveAsWebp(const std::string& inputPath, int quality)
{
    Timer timer;
    Logger::info("Saving image as WebP: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    const std::string output = outputPath(inputPath, "webp_") + ".webp";
    std::vector<int> params = {cv::IMWRITE_WEBP_QUALITY, quality};
    if (!writeImage(output, image, params))
    {
        return Result<std::string>::fail("Failed to save WebP image");
    }

    Logger::info("WebP save completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::saveAsAvif(const std::string& inputPath, int quality)
{
    Timer timer;
    Logger::info("Saving image as AVIF: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    const std::string output = outputPath(inputPath, "avif_") + ".avif";
    std::vector<int> params = {cv::IMWRITE_AVIF_QUALITY, quality};
    if (!writeImage(output, image, params))
    {
        const std::string ffmpegArgs = "-y -i \"" + inputPath + "\" -frames:v 1 -q:v " + std::to_string(quality) + " \"" + output + "\"";
        if (!FFmpeg::run(ffmpegArgs))
        {
            return Result<std::string>::fail("Failed to save AVIF image");
        }
    }

    Logger::info("AVIF save completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}

Result<std::string> ImageProcessor::optimizePng(const std::string& inputPath)
{
    Timer timer;
    Logger::info("Optimizing PNG: " + inputPath);

    cv::Mat image = loadImage(inputPath);
    if (image.empty())
    {
        return Result<std::string>::fail("Unable to load image", StatusCode::FILE_NOT_FOUND);
    }

    const std::string output = outputPath(inputPath, "optimized_") + ".png";
    std::vector<int> params = {cv::IMWRITE_PNG_COMPRESSION, 9};
    if (!writeImage(output, image, params))
    {
        return Result<std::string>::fail("Failed to optimize PNG image");
    }

    Logger::info("PNG optimization completed in " + std::to_string(timer.elapsedMilliseconds()) + " ms");
    return Result<std::string>::ok(output);
}