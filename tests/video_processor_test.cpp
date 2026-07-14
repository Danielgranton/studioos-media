#include <filesystem>
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>

#include <opencv2/opencv.hpp>

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

    auto resizeResult = processor.resize(inputPath.string(), 128, 72);
    if (!resizeResult.success || resizeResult.value.empty() || !fs::exists(resizeResult.value))
    {
        std::cerr << "resize failed: " << resizeResult.message << std::endl;
        return 41;
    }

    const fs::path watermarkPath = tempDir / "watermark.png";
    {
        cv::Mat watermarkImage(24, 24, CV_8UC4, cv::Scalar(0, 255, 0, 120));
        if (!cv::imwrite(watermarkPath.string(), watermarkImage))
        {
            std::cerr << "failed to create watermark image" << std::endl;
            return 42;
        }
    }

    auto watermarkResult = processor.watermark(inputPath.string(), watermarkPath.string(), 4, 4);
    if (!watermarkResult.success || watermarkResult.value.empty() || !fs::exists(watermarkResult.value))
    {
        std::cerr << "watermark failed: " << watermarkResult.message << std::endl;
        return 43;
    }

    auto rotateResult = processor.rotate(inputPath.string(), 90);
    if (!rotateResult.success || rotateResult.value.empty() || !fs::exists(rotateResult.value))
    {
        std::cerr << "rotate failed: " << rotateResult.message << std::endl;
        return 44;
    }

    auto normalizedResult = processor.normalizeAudio(inputPath.string());
    if (!normalizedResult.success || normalizedResult.value.empty() || !fs::exists(normalizedResult.value))
    {
        std::cerr << "audio normalization failed: " << normalizedResult.message << std::endl;
        return 45;
    }

    auto thumbnailsResult = processor.thumbnails(inputPath.string(), 4);
    if (!thumbnailsResult.success || thumbnailsResult.value.empty() || !fs::exists(thumbnailsResult.value))
    {
        std::cerr << "thumbnail set failed: " << thumbnailsResult.message << std::endl;
        return 46;
    }

    std::size_t thumbnailCount = 0;
    for (const auto& entry : fs::directory_iterator(thumbnailsResult.value))
    {
        if (entry.is_regular_file())
        {
            ++thumbnailCount;
        }
    }
    if (thumbnailCount < 4)
    {
        std::cerr << "expected multiple thumbnails" << std::endl;
        return 47;
    }

    auto gifResult = processor.gifPreview(inputPath.string(), 1);
    if (!gifResult.success || gifResult.value.empty() || !fs::exists(gifResult.value))
    {
        std::cerr << "gif preview failed: " << gifResult.message << std::endl;
        return 48;
    }

    auto previewResult = processor.previewClip(inputPath.string(), 1);
    if (!previewResult.success || previewResult.value.empty() || !fs::exists(previewResult.value))
    {
        std::cerr << "preview clip failed: " << previewResult.message << std::endl;
        return 49;
    }

    auto metadataResult = processor.metadata(inputPath.string());
    if (!metadataResult.success || metadataResult.value.width <= 0 || metadataResult.value.height <= 0)
    {
        std::cerr << "metadata extraction failed: " << metadataResult.message << std::endl;
        return 50;
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

    auto frameResult = processor.extractFrame(inputPath.string(), "0.2");
    if (!frameResult.success || frameResult.value.empty() || !fs::exists(frameResult.value))
    {
        std::cerr << "frame extraction failed: " << frameResult.message << std::endl;
        return 15;
    }

    auto adaptiveResult = processor.adaptiveStreaming(inputPath.string(), (tempDir / "abr").string());
    if (!adaptiveResult.success || adaptiveResult.value.empty() || !fs::exists(adaptiveResult.value))
    {
        std::cerr << "adaptive streaming failed: " << adaptiveResult.message << std::endl;
        return 16;
    }

    std::cout << "video processor smoke test passed" << std::endl;
    return 0;
}
