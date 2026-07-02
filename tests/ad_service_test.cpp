#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
}

int main()
{
    const fs::path tempDir = "temp/ad-service-tests";
    fs::create_directories(tempDir);

    const fs::path videoAsset = tempDir / "video.mp4";
    const fs::path imageAsset = tempDir / "image.jpg";
    const fs::path audioAsset = tempDir / "audio.mp3";
    createAsset(videoAsset);
    createAsset(imageAsset);
    createAsset(audioAsset);

    AdService service;

    auto videoResult = service.createVideoAd(videoAsset.string(), "Pre-roll", 15, "https://example.com/video");
    if (!videoResult.success || videoResult.value.empty())
    {
        std::cerr << "video ad creation failed: " << videoResult.message << std::endl;
        return 1;
    }

    auto imageResult = service.createImageAd(imageAsset.string(), "Banner", 8, "https://example.com/image");
    if (!imageResult.success || imageResult.value.empty())
    {
        std::cerr << "image ad creation failed: " << imageResult.message << std::endl;
        return 2;
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
