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

    std::cout << "image processor smoke test passed" << std::endl;
    return 0;
}
