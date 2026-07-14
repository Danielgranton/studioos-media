#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>

#include "services/AdService.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace
{
void createAsset(const fs::path& path)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << "asset";
}

void createVideoAsset(const fs::path& path)
{
    const std::string command =
        "ffmpeg -y -f lavfi -i color=c=blue:s=1280x720:d=10 -f lavfi -i sine=frequency=1000:duration=10 -shortest -pix_fmt yuv420p -c:v libx264 -c:a aac \"" +
        path.string() + "\" >/dev/null 2>&1";
    if (std::system(command.c_str()) != 0)
    {
        std::cerr << "failed to create video asset" << std::endl;
        std::exit(10);
    }
}

void createImageAsset(const fs::path& path)
{
    cv::Mat image(1080, 1920, CV_8UC3, cv::Scalar(20, 20, 220));
    cv::putText(image, "StudioOS Ad", cv::Point(120, 540), cv::FONT_HERSHEY_SIMPLEX, 4.0, cv::Scalar(255, 255, 255), 8);
    if (!cv::imwrite(path.string(), image))
    {
        std::cerr << "failed to create image asset" << std::endl;
        std::exit(11);
    }
}
}

int main()
{
    const fs::path tempDir = "temp/ad-service-tests";
    fs::create_directories(tempDir);

    const fs::path videoAsset = tempDir / "video.mp4";
    const fs::path imageAsset = tempDir / "image.jpg";
    const fs::path audioAsset = tempDir / "audio.mp3";
    createVideoAsset(videoAsset);
    createImageAsset(imageAsset);
    createAsset(audioAsset);

    AdService service;

    auto videoResult = service.createVideoAd(videoAsset.string(), "Pre-roll", 15, "https://example.com/video");
    if (!videoResult.success || videoResult.value.empty())
    {
        std::cerr << "video ad creation failed: " << videoResult.message << std::endl;
        return 1;
    }

    auto processedVideo = service.processVideoAd(videoAsset.string());
    if (!processedVideo.success || !processedVideo.value.valid)
    {
        std::cerr << "video processing failed: " << processedVideo.message << std::endl;
        return 11;
    }

    auto imageResult = service.createImageAd(imageAsset.string(), "Banner", 8, "https://example.com/image");
    if (!imageResult.success || imageResult.value.empty())
    {
        std::cerr << "image ad creation failed: " << imageResult.message << std::endl;
        return 2;
    }

    auto processedImage = service.processImageAd(imageAsset.string());
    if (!processedImage.success || !processedImage.value.valid)
    {
        std::cerr << "image processing failed: " << processedImage.message << std::endl;
        return 12;
    }

    auto audioResult = service.createAudioAd(audioAsset.string(), "Spot", 20, "https://example.com/audio");
    if (!audioResult.success || audioResult.value.empty())
    {
        std::cerr << "audio ad creation failed: " << audioResult.message << std::endl;
        return 3;
    }

    const auto now = std::chrono::system_clock::now();
    auto scheduleResult = service.scheduleAd(
        videoResult.value,
        now - std::chrono::minutes(5),
        now + std::chrono::minutes(5),
        10);
    if (!scheduleResult.success)
    {
        std::cerr << "schedule failed: " << scheduleResult.message << std::endl;
        return 4;
    }

    auto selectResult = service.selectAd(AdService::AdType::Video, now);
    if (!selectResult.success || selectResult.value != videoResult.value)
    {
        std::cerr << "ad selection failed: " << selectResult.message << std::endl;
        return 5;
    }

    if (!service.recordImpression(videoResult.value).success)
    {
        std::cerr << "impression tracking failed" << std::endl;
        return 6;
    }

    if (!service.recordClick(videoResult.value).success)
    {
        std::cerr << "click tracking failed" << std::endl;
        return 7;
    }

    auto reportResult = service.report(videoResult.value);
    if (!reportResult.success)
    {
        std::cerr << "report failed: " << reportResult.message << std::endl;
        return 8;
    }

    const json report = json::parse(reportResult.value);
    if (report.value("impressions", 0) != 1 || report.value("clicks", 0) != 1)
    {
        std::cerr << "unexpected metrics report: " << reportResult.value << std::endl;
        return 9;
    }

    std::cout << "ad service smoke test passed" << std::endl;
    return 0;
}
