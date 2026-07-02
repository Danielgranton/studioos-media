#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "media/ImageProcessor.hpp"

namespace fs = std::filesystem;

int main()
{
    const fs::path tempDir = "temp/tests";
    fs::create_directories(tempDir);

    const fs::path inputPath = tempDir / "sample.jpg";
    {
        cv::Mat image(100, 100, CV_8UC3, cv::Scalar(255, 0, 0));
        if (!cv::imwrite(inputPath.string(), image))
        {
            std::cerr << "failed to create sample image" << std::endl;
            return 1;
        }
    }

    ImageProcessor processor;

    auto compressResult = processor.compress(inputPath.string());
    if (!compressResult.success || compressResult.value.empty())
    {
        std::cerr << "compress failed: " << compressResult.message << std::endl;
        return 1;
    }

    auto resizeResult = processor.resize(inputPath.string(), 100, 100);
    if (!resizeResult.success || resizeResult.value.empty())
    {
        std::cerr << "resize failed: " << resizeResult.message << std::endl;
        return 2;
    }

    auto thumbnailResult = processor.thumbnail(inputPath.string(), 80);
    if (!thumbnailResult.success || thumbnailResult.value.empty())
    {
        std::cerr << "thumbnail failed: " << thumbnailResult.message << std::endl;
        return 3;
    }

    auto cropResult = processor.crop(inputPath.string(), 0, 0, 40, 40);
    if (!cropResult.success || cropResult.value.empty())
    {
        std::cerr << "crop failed: " << cropResult.message << std::endl;
        return 4;
    }

    auto rotateResult = processor.rotate(inputPath.string(), 90.0);
    if (!rotateResult.success || rotateResult.value.empty())
    {
        std::cerr << "rotate failed: " << rotateResult.message << std::endl;
        return 5;
    }

    auto blurResult = processor.blur(inputPath.string(), 3);
    if (!blurResult.success || blurResult.value.empty())
    {
        std::cerr << "blur failed: " << blurResult.message << std::endl;
        return 6;
    }

    const fs::path watermarkPath = tempDir / "watermark.png";
    {
        cv::Mat watermarkImage(20, 20, CV_8UC3, cv::Scalar(0, 255, 0));
        if (!cv::imwrite(watermarkPath.string(), watermarkImage))
        {
            std::cerr << "failed to create watermark image" << std::endl;
            return 6;
        }
    }

    auto watermarkResult = processor.watermark(inputPath.string(), watermarkPath.string(), 5, 5);
    if (!watermarkResult.success || watermarkResult.value.empty())
    {
        std::cerr << "watermark failed: " << watermarkResult.message << std::endl;
        return 7;
    }

    auto sharpenResult = processor.sharpen(inputPath.string(), 1.2);
    if (!sharpenResult.success || sharpenResult.value.empty())
    {
        std::cerr << "sharpen failed: " << sharpenResult.message << std::endl;
        return 8;
    }

    auto metadataResult = processor.removeMetadata(inputPath.string());
    if (!metadataResult.success || metadataResult.value.empty())
    {
        std::cerr << "metadata removal failed: " << metadataResult.message << std::endl;
        return 9;
    }

    auto webpResult = processor.saveAsWebp(inputPath.string(), 80);
    if (!webpResult.success || webpResult.value.empty())
    {
        std::cerr << "webp failed: " << webpResult.message << std::endl;
        return 10;
    }

    auto avifResult = processor.saveAsAvif(inputPath.string(), 70);
    if (!avifResult.success || avifResult.value.empty())
    {
        std::cerr << "avif failed: " << avifResult.message << std::endl;
        return 11;
    }

    auto pngResult = processor.optimizePng(inputPath.string());
    if (!pngResult.success || pngResult.value.empty())
    {
        std::cerr << "png failed: " << pngResult.message << std::endl;
        return 12;
    }

    std::cout << "image processor smoke test passed" << std::endl;
    return 0;
}
