#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "media/AudioProcessor.hpp"

namespace fs = std::filesystem;

int main()
{
    const fs::path tempDir = "temp/tests";
    fs::create_directories(tempDir);

    const fs::path inputPath = tempDir / "sample.wav";
    const std::string generateCommand =
        "ffmpeg -y -f lavfi -i sine=frequency=1000:duration=1 -acodec pcm_s16le " + inputPath.string() + " >/dev/null 2>&1";
    if (std::system(generateCommand.c_str()) != 0)
    {
        std::cerr << "failed to generate sample audio" << std::endl;
        return 1;
    }

    AudioProcessor processor;

    auto encodeResult = processor.encode(inputPath.string(), "mp3");
    if (!encodeResult.success || encodeResult.value.empty())
    {
        std::cerr << "encode failed: " << encodeResult.message << std::endl;
        return 2;
    }

    auto normalizeResult = processor.normalize(inputPath.string());
    if (!normalizeResult.success || normalizeResult.value.empty())
    {
        std::cerr << "normalize failed: " << normalizeResult.message << std::endl;
        return 3;
    }

    auto denoiseResult = processor.denoise(inputPath.string());
    if (!denoiseResult.success || denoiseResult.value.empty())
    {
        std::cerr << "denoise failed: " << denoiseResult.message << std::endl;
        return 4;
    }

    const std::vector<std::string> mergeInputs = {inputPath.string(), inputPath.string()};
    const fs::path mergedPath = tempDir / "merged.mp3";
    auto mergeResult = processor.merge(mergeInputs, mergedPath.string());
    if (!mergeResult.success || mergeResult.value.empty())
    {
        std::cerr << "merge failed: " << mergeResult.message << std::endl;
        return 5;
    }

    auto trimResult = processor.trim(inputPath.string(), "00:00:00", "00:00:00.5");
    if (!trimResult.success || trimResult.value.empty())
    {
        std::cerr << "trim failed: " << trimResult.message << std::endl;
        return 6;
    }

    auto convertResult = processor.convert(inputPath.string(), "aac");
    if (!convertResult.success || convertResult.value.empty())
    {
        std::cerr << "convert failed: " << convertResult.message << std::endl;
        return 7;
    }

    std::cout << "audio processor smoke test passed" << std::endl;
    return 0;
}
