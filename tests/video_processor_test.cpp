#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "media/VideoProcessor.hpp"

namespace fs = std::filesystem;

int main()
{
    const fs::path tempDir = "temp/tests";
    fs::create_directories(tempDir);

    const fs::path inputPath = tempDir / "sample.mp4";
    {
        std::ofstream stream(inputPath, std::ios::binary);
        stream << "dummy video data";
    }

    VideoProcessor processor;

    auto compressResult = processor.compress(inputPath.string());
    if (!compressResult.success || compressResult.value.empty())
    {
        std::cerr << "compress failed: " << compressResult.message << std::endl;
        return 1;
    }

    auto thumbnailResult = processor.thumbnail(inputPath.string());
    if (!thumbnailResult.success || thumbnailResult.value.empty())
    {
        std::cerr << "thumbnail failed: " << thumbnailResult.message << std::endl;
        return 2;
    }

    auto audioResult = processor.extractAudio(inputPath.string());
    if (!audioResult.success || audioResult.value.empty())
    {
        std::cerr << "audio extraction failed: " << audioResult.message << std::endl;
        return 3;
    }

    std::cout << "video processor smoke test passed" << std::endl;
    return 0;
}
