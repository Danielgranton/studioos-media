#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

#include "core/Result.hpp"
#include "media/VideoProcessor.hpp"

class AdService
{
public:
    enum class AdType
    {
        Video,
        Image,
        Audio
    };

    struct AdMetrics
    {
        std::size_t impressions = 0;
        std::size_t clicks = 0;
    };

    struct ProcessedAd
    {
        std::string originalAsset;
        std::string normalizedVideo;
        std::string variant1080;
        std::string variant720;
        std::string variant480;
        std::string variant360;
        std::string hlsManifest;
        std::string dashManifest;
        std::string thumbnail;
        std::string poster;
        std::string preview;
        std::string metadataJson;
        std::string checksum;
        double qualityScore = 0.0;
        bool valid = false;
        std::vector<std::string> warnings;
    };

    Result<std::string> createVideoAd(
        const std::string& assetPath,
        const std::string& title,
        int durationSeconds,
        const std::string& clickUrl = "");

    Result<std::string> createImageAd(
        const std::string& assetPath,
        const std::string& title,
        int durationSeconds,
        const std::string& clickUrl = "");

    Result<std::string> createAudioAd(
        const std::string& assetPath,
        const std::string& title,
        int durationSeconds,
        const std::string& clickUrl = "");

    Result<std::string> scheduleAd(
        const std::string& adId,
        const std::chrono::system_clock::time_point& startsAt,
        const std::chrono::system_clock::time_point& endsAt,
        int priority = 0);

    Result<std::string> selectAd(
        AdType type,
        const std::chrono::system_clock::time_point& at = std::chrono::system_clock::now());

    Result<std::string> recordImpression(const std::string& adId);

    Result<std::string> recordClick(const std::string& adId);

    Result<std::string> report(const std::string& adId) const;

    Result<ProcessedAd> validateVideoAd(const std::string& assetPath);

    Result<ProcessedAd> processVideoAd(const std::string& assetPath);

    Result<ProcessedAd> validateImageAd(const std::string& assetPath);

    Result<ProcessedAd> processImageAd(const std::string& assetPath);

private:
    struct AdCreative
    {
        std::string id;
        AdType type;
        std::string assetPath;
        std::string title;
        std::string clickUrl;
        int durationSeconds = 0;
        ProcessedAd processed;
    };

    struct AdSchedule
    {
        std::string adId;
        std::chrono::system_clock::time_point startsAt;
        std::chrono::system_clock::time_point endsAt;
        int priority = 0;
    };

    Result<std::string> createAd(
        AdType type,
        const std::string& assetPath,
        const std::string& title,
        int durationSeconds,
        const std::string& clickUrl);

    static std::string typeToString(AdType type);

    static bool isActive(
        const AdSchedule& schedule,
        const std::chrono::system_clock::time_point& at);

    std::string nextId();

    mutable std::mutex mMutex;
    std::unordered_map<std::string, AdCreative> mAds;
    std::unordered_map<std::string, AdSchedule> mSchedules;
    std::unordered_map<std::string, AdMetrics> mMetrics;
    std::size_t mNextId = 1;
};
