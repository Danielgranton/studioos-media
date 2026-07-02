#include <filesystem>
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>

#include "media/VideoProcessor.hpp"

namespace fs = std::filesystem;

int main()
{
    const fs::path tempDir = "temp/tests";
    fs::create_directories(tempDir);

    const fs::path inputPath = tempDir / "sample.mp4";
    const std::string generateCommand =
        "ffmpeg -y -f lavfi -i color=c=blue:s=160x120:d=1 -f lavfi -i sine=frequency=1000:duration=1 -shortest -pix_fmt yuv420p -c:v libx264 -c:a aac " + inputPath.string() + " >/dev/null 2>&1";
    if (std::system(generateCommand.c_str()) != 0)
    {
        std::cerr << "failed to generate sample video" << std::endl;
        return 1;
    }

    if (!fs::exists(inputPath) || fs::file_size(inputPath) == 0)
    {
        std::cerr << "generated sample video is missing or empty" << std::endl;
        return 2;
    }

    VideoProcessor processor;

    auto compressResult = processor.compress(inputPath.string());
    if (!compressResult.success || compressResult.value.empty())
    {
        std::cerr << "compress failed: " << compressResult.message << std::endl;
        return 3;
    }

    if (!fs::exists(compressResult.value) || fs::file_size(compressResult.value) == 0)
    {
        std::cerr << "compressed output was not created" << std::endl;
        return 4;
    }

    auto thumbnailResult = processor.thumbnail(inputPath.string());
    if (!thumbnailResult.success || thumbnailResult.value.empty())
    {
        std::cerr << "thumbnail failed: " << thumbnailResult.message << std::endl;
        return 5;
    }

    if (!fs::exists(thumbnailResult.value) || fs::file_size(thumbnailResult.value) == 0)
    {
        std::cerr << "thumbnail output was not created" << std::endl;
        return 6;
    }

    auto audioResult = processor.extractAudio(inputPath.string());
    if (!audioResult.success || audioResult.value.empty())
    {
        std::cerr << "audio extraction failed: " << audioResult.message << std::endl;
        return 7;
    }

    if (!fs::exists(audioResult.value) || fs::file_size(audioResult.value) == 0)
    {
        std::cerr << "audio output was not created" << std::endl;
        return 8;
    }

    auto convertResult = processor.convert(inputPath.string(), "mov");
    if (!convertResult.success || convertResult.value.empty())
    {
        std::cerr << "conversion failed: " << convertResult.message << std::endl;
        return 9;
    }

    if (!fs::exists(convertResult.value) || fs::file_size(convertResult.value) == 0)
    {
        std::cerr << "converted output was not created" << std::endl;
        return 10;
    }

    auto trimResult = processor.trim(inputPath.string(), "00:00:00", "00:00:00.5");
    if (!trimResult.success || trimResult.value.empty())
    {
        std::cerr << "trim failed: " << trimResult.message << std::endl;
        return 11;
    }

    if (!fs::exists(trimResult.value) || fs::file_size(trimResult.value) == 0)
    {
        std::cerr << "trimmed output was not created" << std::endl;
        return 12;
    }

    const std::vector<std::string> mergeInputs = {inputPath.string(), inputPath.string()};
    const fs::path mergedPath = tempDir / "merged.mp4";
    auto mergeResult = processor.merge(mergeInputs, mergedPath.string());
    if (!mergeResult.success || mergeResult.value.empty())
    {
        std::cerr << "merge failed: " << mergeResult.message << std::endl;
        return 13;
    }

    if (!fs::exists(mergeResult.value) || fs::file_size(mergeResult.value) == 0)
    {
        std::cerr << "merged output was not created" << std::endl;
        return 14;
    }

    std::cout << "video processor smoke test passed" << std::endl;
    return 0;
}
