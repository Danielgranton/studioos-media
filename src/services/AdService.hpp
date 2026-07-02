#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

#include "core/Result.hpp"

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

private:
    struct AdCreative
    {
        std::string id;
        AdType type;
        std::string assetPath;
        std::string title;
        std::string clickUrl;
        int durationSeconds = 0;
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
